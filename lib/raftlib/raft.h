#ifndef RAFT_H
#define RAFT_H

#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include "sync_queue.h"
#include "logcat.h"
#include "mac.h"
#include "entry.h"
#include <random>

#define AP_ENTRY_REQUEST_CODE 0
#define AP_ENTRY_RESPONSE_CODE 1
#define RE_VOTE_REQUEST_CODE 2
#define RE_VOTE_RESPONSE_CODE 3

static thread_local std::mt19937 generator;

enum class State {
    FOLLOWER, CANDIDATE, LEADER
};

std::string stateToStr(State state);


/** This struct leaves ~228 bytes worth of space for entries */
struct AppendEntryRequest {
    int term;                       // Term of the leader that send this request
    MACAddress leaderID;            // The current leader
    int prevLogIndex;               // The index of the last log before these new ones
    int prevLogTerm;                // The term of the last log before these new ones
    std::vector<Entry> entries;     // List of entries to append
    int leaderCommit;               // The Leader's commit index
};

struct AppendEntryResponse {
    int term;               // This node's term
    MACAddress senderID;    // The sender's id
    bool success;           // True if this node's blockchain matches the leader's
};

struct RequestVoteRequest {
    int term;                       // Term of the candidate that sent this request
    MACAddress candidateID;         // The candidate that requested this vote
    int LastLogIndex;               // Index of the candidate's last log entry
    int lastLogTerm;                // Term of the candidate's last log entry
};

struct RequestVoteResponse {
    int term;              // This node's term
    bool voteGranted;       // True if the node recieved this node's vote
};

/** Represents a RAFT Node */
class RaftServer {
    private:
        int m_currentTerm;
        MACAddress m_votedFor;         // For the purpose of this program, it is assumed that a MAC address of 00:00:00:00:00:00 is null
        MACAddress m_lastKnownLeader; 
        int m_votesRecieved;

        long long m_nextEpoch;        // The time at which the next important thing happens

        std::vector<Entry> m_log;

        int m_commitIndex;
        State m_currentState;

        SyncQueue<AppendEntryRequest> m_appendEntryRequests;
        SyncQueue<AppendEntryResponse> m_appendEntryResponses;
        SyncQueue<RequestVoteRequest> m_requestVoteRequests;
        SyncQueue<RequestVoteResponse> m_requestVoteResponse;

        AppendEntryResponse respondToAppendRequest(const AppendEntryRequest&);

        RequestVoteResponse respondToVoteRequest(const RequestVoteRequest&);

        inline bool isMoreUpToDate(int lastLogTerm, int lastLogIndex);

        bool doFollowerLogic();

        void startNewCandidateCycle(long long currentTime);
        
        void resetToFollower(long long currentTime, int newTerm);

        long long getElectionTimeout() {
            std::uniform_int_distribution<int> distribution(200,350);
            return distribution(generator);
        }

        long long getHeartbeatTimeout() {return 60LL; };

        // These next few are just for the leader
        std::vector<int> m_nextIndex;
        std::vector<int> m_matchIndex;

        // These all do what you think they do
        void sendMessage(MACAddress recipientID, const AppendEntryRequest& msg);
        void sendMessage(MACAddress recipientID, const RequestVoteRequest& msg);
        void sendMessage(MACAddress recipientID, const AppendEntryResponse& msg);
        void sendMessage(MACAddress recipientID, const RequestVoteResponse& msg);

        int newCommitIndex();

    protected:
        std::vector<MACAddress> m_otherServers;
        MACAddress m_serverID;

    public:

        void addMessage(const AppendEntryRequest& msg);
        void addMessage(const RequestVoteRequest& msg);
        void addMessage(const AppendEntryResponse& msg);
        void addMessage(const RequestVoteResponse& msg);

        void addLogEntry(Entry entry) {
            m_log.emplace_back(entry);
        }

        void initializeRaftServer(long long currentTime, MACAddress serverID, const std::vector<MACAddress>& otherMembers);

        void updateRaftServer(long long currentTime);

        State getCurrentState() { return m_currentState; }
        int getCommitIndex() { return m_commitIndex; }
        int getLogSize() { return m_log.size(); }
        int getCurrentTerm() { return m_currentTerm; }
        Entry getLogEntry(int index) { return m_log[index]; }
};

#endif