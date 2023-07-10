#include <iostream>
#include "raft.h" 


std::string stateToStr(State state) {
    switch (state) {
        case State::FOLLOWER: return "Follower";
        case State::CANDIDATE: return "Candidate";
        case State::LEADER: return "Leader";
        default: return "Unknown";
    }
}

void RaftServer::updateRaftServer(long long currentTime) {
    //*************************** FOLLOWER LOGIC ***************************//
    if (m_currentState == State::FOLLOWER) {
        bool resetTimer = doFollowerLogic();

        if (resetTimer) {
            m_nextEpoch = currentTime + getElectionTimeout();
        } 
        else if (currentTime >= m_nextEpoch) {
            startNewCandidateCycle(currentTime);
        }
    }

    //*************************** CANDIDATE LOGIC ***************************//
    if (m_currentState == State::CANDIDATE) {
        while (!m_appendEntryRequests.empty()) {
            AppendEntryRequest request = m_appendEntryRequests.get();
            if (request.term >= m_currentTerm) {
                resetToFollower(currentTime, request.term);
                return; // Exit out of the update and handle other pending entry requests in the next update cycle.
            }
        }
        while (!m_requestVoteRequests.empty()) {
            RequestVoteRequest request = m_requestVoteRequests.get();
            if (request.term > m_currentTerm) {
                resetToFollower(currentTime, request.term);
                return; // Exit out of the update and handle other pending entry requests in the next update cycle.
            }
        }

        while (!m_requestVoteResponse.empty()) {
            RequestVoteResponse response = m_requestVoteResponse.get();
            if (response.term == m_currentTerm and response.voteGranted) m_votesRecieved++;
        }

        if (m_votesRecieved*2 > m_otherServers.size()) {
            // Promote to leader
            m_currentState = State::LEADER;
            m_lastKnownLeader = m_serverID;
            m_nextEpoch = currentTime + getHeartbeatTimeout();

            m_nextIndex.clear(); m_matchIndex.clear();
            m_nextIndex.resize(m_otherServers.size(), m_log.size());
            m_matchIndex.resize(m_otherServers.size(), 0);
            for (MACAddress i : m_otherServers) {
                sendMessage(i, AppendEntryRequest{m_currentTerm, m_serverID, (int) m_log.size()-1, (m_log.empty())?0:m_log.back().term, std::vector<Entry>(0), m_commitIndex});
            }
        } else if (currentTime >= m_nextEpoch) startNewCandidateCycle(currentTime);
    }

    //*************************** LEADER LOGIC ***************************//
    if (m_currentState == State::LEADER) {
        while(!m_appendEntryRequests.empty()) {
            AppendEntryRequest request = m_appendEntryRequests.get();
            if (request.term > m_currentTerm) {
                resetToFollower(currentTime, request.term);
                return; // Exit out of the update and handle other pending entry requests in the next update cycle.
            }
        }
        while (!m_requestVoteRequests.empty()) {
            RequestVoteRequest request = m_requestVoteRequests.get();
            if (request.term > m_currentTerm) {
                resetToFollower(currentTime, request.term);
                return; // Exit out of the update and handle other pending entry requests in the next update cycle.
            }
        }
        while (!m_appendEntryResponses.empty()) {
            AppendEntryResponse response = m_appendEntryResponses.get();
            int indexInServerList = -1;
            for (int i = 0; i < m_otherServers.size(); i++) { 
                if (m_otherServers[i] == response.senderID) {
                    indexInServerList = i;
                    break;
                }
            }
            if (indexInServerList == -1) continue;
            if (response.success) {
                m_nextIndex[indexInServerList] = m_log.size();
                m_matchIndex[indexInServerList] = m_log.size() - 1;
            } else {
                m_nextIndex[indexInServerList]--;
            }
        }

        for (int i = 0; i < m_otherServers.size(); i++) {
            if (m_log.size() > m_nextIndex[i]) 
                sendMessage(m_otherServers[i], {m_currentTerm, m_serverID, m_nextIndex[i]-1, (m_nextIndex[i]>0)?m_log[m_nextIndex[i]-1].term:0, std::vector<Entry>(m_log.begin() + m_nextIndex[i], m_log.end()), m_commitIndex});
            else if (m_nextEpoch <= currentTime) {
                sendMessage(m_otherServers[i], AppendEntryRequest{m_currentTerm, m_serverID, (int) m_log.size()-1, (m_log.empty())?0:m_log.back().term, std::vector<Entry>(0), m_commitIndex});
            }
        }

        if (m_nextEpoch <= currentTime) m_nextEpoch = currentTime + getHeartbeatTimeout();

        // Update the commit index if needed
        m_commitIndex = newCommitIndex();


    }

    // Auto-clear everything (unless the control loop was exited earlier)
    m_appendEntryRequests.clear();
    m_appendEntryResponses.clear();
    m_requestVoteRequests.clear();
    m_requestVoteResponse.clear();
}

int RaftServer::newCommitIndex() {
    int highestWorking = m_commitIndex;
    // Update the commit index 
    for (int newCommitIndex = m_commitIndex + 1; newCommitIndex < m_log.size(); newCommitIndex++) {
        if (m_log[newCommitIndex].term != m_currentTerm) continue;
        int counter = 0;
        for (int i : m_matchIndex) if (i >= m_commitIndex) counter++;
        if (counter*2 <= m_otherServers.size() + 1) break;
        highestWorking = newCommitIndex;
    }
    return highestWorking;
}

void RaftServer::initializeRaftServer(long long currentTime, MACAddress serverID, const std::vector<MACAddress>& otherMembers) {
    // First, a bunch of variables:
    m_currentTerm = 0;
    m_votedFor = MACAddress(0, 0, 0, 0, 0, 0);
    m_log = std::vector<Entry>(0);
    m_commitIndex = -1;
    m_currentState = State::FOLLOWER;
    m_lastKnownLeader = MACAddress(0, 0, 0, 0, 0, 0); 
    m_serverID = serverID;

    m_nextEpoch = currentTime + getElectionTimeout();

    m_otherServers = otherMembers; // Copy server list

    Log::log(m_serverID.print(), "Create server with this many peers", (int)m_otherServers.size());
}

void RaftServer::resetToFollower(long long currentTime, int newTerm) {
    if(newTerm > m_currentTerm) m_votedFor = MACAddress(0, 0, 0, 0, 0, 0);

    m_currentState = State::FOLLOWER;
    m_currentTerm = newTerm;
    m_nextEpoch = currentTime + getElectionTimeout();
}

void RaftServer::startNewCandidateCycle(long long currentTime) {
    // Convert to candidate
    m_currentState = State::CANDIDATE;
    m_currentTerm++;
    m_nextEpoch = currentTime + getElectionTimeout();
    m_votesRecieved = 1;
    m_votedFor = m_serverID;

    for (MACAddress i : m_otherServers) {
        sendMessage(i, RequestVoteRequest{m_currentTerm, m_serverID, (int)m_log.size()-1, (m_log.empty())?0:m_log.back().term});
    }
}

/** Does all of the follower logic specified in fig.2 of the RAFT paper.
 * NOTE: This method does NOT handle state transistions
 * 
 *  @return        true if the election timeout should be reset. */
bool RaftServer::doFollowerLogic() {
    bool resetTimer = false;

    while (!m_appendEntryRequests.empty()) {
        AppendEntryRequest request = m_appendEntryRequests.get();
        if (request.term >= m_currentTerm) { // As long as this is a valid leader
            resetTimer = true;
            m_lastKnownLeader = request.leaderID;
            if (request.term > m_currentTerm) m_votedFor = MACAddress(0, 0, 0, 0, 0, 0);
            m_currentTerm = request.term;
        }
        sendMessage(request.leaderID, respondToAppendRequest(request));
    }

    while (!m_requestVoteRequests.empty()) {
        RequestVoteRequest request = m_requestVoteRequests.get();
        if (request.term > m_currentTerm) {
            m_currentTerm = request.term;
            m_votedFor = MACAddress(0, 0, 0, 0, 0, 0);
        }
        RequestVoteResponse response = respondToVoteRequest(request);
        if (response.voteGranted) resetTimer = true;
        sendMessage(request.candidateID, response);
    }

    return resetTimer;
}

/** Handles all of the response logic found in the AppendEntries RPC box in fig. 2 of the RAFT paper */
AppendEntryResponse RaftServer::respondToAppendRequest(const AppendEntryRequest& rpc) {
    if (rpc.term < m_currentTerm) return {m_currentTerm, m_serverID, false};   // This leader is from an older term

    int current_sz = m_log.size();
    if (current_sz < rpc.prevLogIndex+1 or (rpc.prevLogIndex >= 0 and m_log[rpc.prevLogIndex].term != rpc.prevLogTerm)) return {m_currentTerm, m_serverID, false}; // Not up-to-date

    // Remove the extra stuff that doesn't match the append request
    for (int i = current_sz-1; i > rpc.prevLogIndex; i--) { // Iterate through log entries in reverse order
        if (i > rpc.entries.size()+rpc.prevLogIndex or m_log[i].term != rpc.entries[i-rpc.prevLogIndex-1].term) current_sz = i;
    }
    if (current_sz != m_log.size()) m_log.erase(m_log.begin() + current_sz, m_log.end());

    // Add any extra logs that are not already part of the chain
    m_log.insert(m_log.end(), rpc.entries.begin() + (current_sz - rpc.prevLogIndex - 1), rpc.entries.end());

    // Update commit status
    if (rpc.leaderCommit > m_commitIndex) m_commitIndex = (rpc.leaderCommit <= (m_log.size()-1))?rpc.leaderCommit:(m_log.size()-1);

    return {m_currentTerm, m_serverID, true};
}

/** Handles all of the response logic found in the RequestVote RPC box in fig. 2 of the RAFT paper */
RequestVoteResponse RaftServer::respondToVoteRequest(const RequestVoteRequest& rpc) {
    if (rpc.term < m_currentTerm) return {m_currentTerm, false};
    if ((m_votedFor.isNull()) and isMoreUpToDate(rpc.lastLogTerm, rpc.LastLogIndex)) {
        m_votedFor = rpc.candidateID;
        return {m_currentTerm, true};
    } 
    return {m_currentTerm, false};
}

/** Returns true if the given parameters are at least as up-to-date as this node's log */
inline bool RaftServer::isMoreUpToDate(int lastLogTerm, int lastLogIndex) {
    if (m_log.empty()) return (lastLogIndex == -1);
    if (lastLogTerm == m_log.back().term) return (lastLogIndex >= (m_log.size() - 1));
    else return (lastLogTerm > m_log.back().term);
}

/** The next few are just the various message recievers */
void RaftServer::addMessage(const AppendEntryRequest& msg) { m_appendEntryRequests.add(msg); }
void RaftServer::addMessage(const RequestVoteRequest& msg) { m_requestVoteRequests.add(msg); }
void RaftServer::addMessage(const AppendEntryResponse& msg) { m_appendEntryResponses.add(msg); }
void RaftServer::addMessage(const RequestVoteResponse& msg) { m_requestVoteResponse.add(msg); }