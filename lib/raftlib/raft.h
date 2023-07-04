#ifndef RAFT_H
#define RAFT_H

#include <vector>
#include <algorithm>
#include "sync_queue.h"

enum class State {
    FOLLOWER, CANDIDATE, LEADER
};

struct Entry {
    int term;
};

/** Represents a 42bit MAC address: a(32-bit) b(16-bit)*/
struct MACAddress {
    uint8_t address[6];

    /** Shorthand for convenience */
    MACAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f) { 
        address[0] = a; address[1] = b; address[2] = c; address[3] = d; address[4] = e; address[5] = f;
    }

    bool operator==(const MACAddress& other) {
        return address[0] == other.address[0]
           and address[1] == other.address[1]
           and address[2] == other.address[2]
           and address[3] == other.address[3]
           and address[4] == other.address[4]
           and address[5] == other.address[5];
    } 

    bool isNull() { 
        return address[0] == 0
           and address[1] == 0
           and address[2] == 0
           and address[3] == 0
           and address[4] == 0
           and address[5] == 0;
    }
};

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

         long long getElectionTimeout();
        long long getHeartbeatTimeout();

        // These next few are just for the leader
        std::vector<int> m_nextIndex;
        std::vector<int> m_matchIndex;

        // These all do what you think they do
        virtual void sendMessage(MACAddress recipientID, const AppendEntryRequest& msg);
        virtual void sendMessage(MACAddress recipientID, const RequestVoteRequest& msg);
        virtual void sendMessage(MACAddress recipientID, const AppendEntryResponse& msg);
        virtual void sendMessage(MACAddress recipientID, const RequestVoteResponse& msg);

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
};

#endif