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

/** Represents a 42bit MAC address: a(32-bit) b(16-bit)*/
struct MACAddress {
    uint32_t a;
    uint16_t b;

    /** Shorthand for convenience */
    MACAddress(uint8_t _a, uint8_t _b, uint8_t _c, uint8_t _d, uint8_t _e, uint8_t _f) : 
        a((((uint32_t)_a)<<24) + (((uint32_t)_b)<<16) + (((uint32_t)_c)<<8) + (uint32_t)_d), 
        b(((uint16_t)_e<<8) + (uint16_t)_f) {}

    bool operator==(const MACAddress& other) {
        return a==other.a and b==other.b;
    } 

    bool isNull() { return a==0 and b==0; }
};

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
        MACAddress m_serverID;
        std::vector<MACAddress> m_otherServers;

        int m_currentTerm;
        MACAddress m_votedFor;         // For the purpose of this program, it is assumed that a MAC address of 00:00:00:00:00:00 is null
        MACAddress m_lastKnownLeader; 
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
        virtual void sendMessage(MACAddress recipientID, const AppendEntryRequest& msg);
        virtual void sendMessage(MACAddress recipientID, const RequestVoteRequest& msg);
        virtual void sendMessage(MACAddress recipientID, const AppendEntryResponse& msg);
        virtual void sendMessage(MACAddress recipientID, const RequestVoteResponse& msg);

        void addMessage(const AppendEntryRequest& msg);
        void addMessage(const RequestVoteRequest& msg);
        void addMessage(const AppendEntryResponse& msg);
        void addMessage(const RequestVoteResponse& msg);

        void addLogEntry(Entry entry) {
            m_log.emplace_back(entry);
        }

        void initialize(long long currentTime, MACAddress serverID, const std::vector<MACAddress>& otherMembers);

        void update(long long currentTime);
};

#endif