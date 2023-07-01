#ifndef RAFT_H
#define RAFT_H

#include <vector>
#include <algorithm>

enum class State {
    FOLLOWER, CANDIDATE, LEADER
};

struct Entry {
    int term;
};

struct AppendEntryRequest {
    int term;                       // Term of the leader that send this request
    int leaderID;                   // The current leader
    int prevLogIndex;               // The index of the last log before these new ones
    int prevLogTerm;                // The term of the last log before these new ones
    std::vector<Entry> entries;     // List of entries to append
    int leaderCommit;               // The Leader's commit index
};

struct AppendEntryResponse {
    int term;       // This node's term
    int senderID;   // The sender's id
    bool success;   // True if this node's blockchain matches the leader's
};

struct RequestVoteRequest {
    int term;                       // Term of the candidate that sent this request
    int candidateID;                // The candidate that requested this vote
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
        int m_serverID;
        std::vector<int> m_otherServers;

        int m_currentTerm;
        int m_votedFor;         // -1 refer to not voted
        int m_lastKnownLeader; 
        int m_votesRecieved;

        long long m_nextEpoch;        // The time at which the next important thing happens

        std::vector<Entry> m_log;

        int m_commitIndex;
        State m_currentState;

        std::vector<AppendEntryRequest> m_appendEntryRequests;
        std::vector<AppendEntryResponse> m_appendEntryResponses;
        std::vector<RequestVoteRequest> m_requestVoteRequests;
        std::vector<RequestVoteResponse> m_requestVoteResponse;

        AppendEntryResponse respondToAppendRequest(const AppendEntryRequest&);

        RequestVoteResponse respondToVoteRequest(const RequestVoteRequest&);

        inline bool isMoreUpToDate(int lastLogTerm, int lastLogIndex);

        bool doFollowerLogic(const std::vector<AppendEntryRequest>& appendEntryRPCs, const std::vector<RequestVoteRequest>& voteRequestRPCs);

        void startNewCandidateCycle(long long currentTime);
        
        void resetToFollower(long long currentTime, int newTerm);

        long long getElectionTimeout();
        long long getHeartbeatTimeout();

        // These next few are just for the leader
        std::vector<int> m_nextIndex;
        std::vector<int> m_matchIndex;

    public:
        // These all do what you think they do
        void sendMessage(int recipientID, const AppendEntryRequest& msg);
        void sendMessage(int recipientID, const RequestVoteRequest& msg);
        void sendMessage(int recipientID, const AppendEntryResponse& msg);
        void sendMessage(int recipientID, const RequestVoteResponse& msg);

        void addMessage(const AppendEntryRequest& msg);
        void addMessage(const RequestVoteRequest& msg);
        void addMessage(const AppendEntryResponse& msg);
        void addMessage(const RequestVoteResponse& msg);

        void addLogEntry(Entry entry) {
            m_log.emplace_back(entry);
        }

        void initialize(long long currentTime, int serverID, const std::vector<int>& otherMembers);

        void update(long long currentTime);
};

#endif