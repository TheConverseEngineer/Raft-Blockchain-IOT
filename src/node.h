#ifndef NODE_H
#define NODE_H

#include "raft.h"
#include <utility>
#include <iostream>
#include "sha256.h"

#define IS_SIMULATION

#ifndef IS_SIMULATION
    #include <esp_now.h>
    #include <WiFi.h>
#else
    #include "fake_esp_now.h"

#endif

#define STORAGE_REQUEST_CODE 4
#define STORAGE_RESPONSE_CODE 5
#define DATA_DELIVERY_CODE 6
#define DATA_CONFIRMATION_CODE 7

const long long TIME_TO_WAIT_FOR_STORAGE_RESPONSE = 200; //ms
const long long DATA_UPDATE_PULSE = 300; //ms

struct StorageRequest {
    MACAddress senderID;
};

struct StorageResponse {
    MACAddress senderID;
    bool canStore;
};

struct Data {
    long long timestamp;
    unsigned char data[];
};

struct DataDelivery {
    MACAddress sender;
    Data data;
};

struct DataConfirmation {
    MACAddress sender;
    bool successful;
    Hash dataHash;
};

struct DataLinkedList {
    DataLinkedList* next;
    Data data;
};

const long long STORAGE_RESET_DURATION = 10000; //ms

class Node : public RaftServer {
    private:
        long long m_responseTimeline;
        long long m_responseTimeout;

        long long m_nextDataUpdateTime;

        void sendMessage(MACAddress recipient, const StorageRequest& msg);
        void sendMessage(MACAddress recipient, const StorageResponse& msg);

        SyncQueue<StorageRequest> m_storageRequests;
        SyncQueue<StorageResponse> m_storageResponse;

        std::vector<Data> m_securedData;
        int m_dataSecuredIndex;
        DataLinkedList* m_pendingData;

    public:
        void addMessage(const StorageRequest& msg) { m_storageRequests.add(msg); }
        void addMessage(const StorageResponse& msg) { m_storageResponse.add(msg); }

        void onRecvData(const uint8_t* mac_addr, const uint8_t* data, int data_len);

        void update(long long currentTime);
        void begin(long long currentTime, MACAddress serverID, const std::vector<MACAddress>& otherMembers);

        Node() {}
        ~Node() = default;
};

#endif