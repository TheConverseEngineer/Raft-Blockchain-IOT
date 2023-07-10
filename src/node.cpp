#include "node.h"

void RaftServer::sendMessage(MACAddress recipient, const RequestVoteRequest& msg) { 
    // Log::log(m_serverID, "Sending VoteRequest message to", recipient);
    std::pair<uint8_t, RequestVoteRequest> data_pair = {RE_VOTE_REQUEST_CODE, msg};
    esp_now_send(recipient.address, (uint8_t *) &data_pair, sizeof(data_pair));
}

void RaftServer::sendMessage(MACAddress recipient, const AppendEntryRequest& msg)  { 
    //Log::log(m_serverID, "Sending AppendRequest message to", recipient);
    std::pair<uint8_t, AppendEntryRequest> data_pair = {AP_ENTRY_REQUEST_CODE, msg};
    esp_now_send(&recipient.address[0], (uint8_t *) &data_pair, sizeof(data_pair));
}

void RaftServer::sendMessage(MACAddress recipient, const RequestVoteResponse& msg)  { 
    // Log::log(m_serverID, "Sending VoteResponse message to", recipient);
    std::pair<uint8_t, RequestVoteResponse> data_pair = {RE_VOTE_RESPONSE_CODE, msg};
    esp_now_send(&recipient.address[0], (uint8_t *) &data_pair, sizeof(data_pair));
}

void RaftServer::sendMessage(MACAddress recipient, const AppendEntryResponse& msg)  { 
    //Log::log(m_serverID, "Sending AppendResponse message to", recipient);
    std::pair<uint8_t, AppendEntryResponse> data_pair = {AP_ENTRY_RESPONSE_CODE, msg};
    esp_now_send(&recipient.address[0], (uint8_t *) &data_pair, sizeof(data_pair));
}

void Node::sendMessage(MACAddress recipient, const StorageRequest& msg) {
    std::pair<uint8_t, StorageRequest> data_pair = {STORAGE_REQUEST_CODE, msg};
    esp_now_send(&recipient.address[0], (uint8_t *) &data_pair, sizeof(data_pair));
}

void Node::sendMessage(MACAddress recipient, const StorageResponse& msg) {
    std::pair<uint8_t, StorageResponse> data_pair = {STORAGE_RESPONSE_CODE, msg};
    esp_now_send(&recipient.address[0], (uint8_t *) &data_pair, sizeof(data_pair));
}

void Node::onRecvData(const uint8_t* mac_addr, const uint8_t* data, int data_len) {
    switch (int(*data)) {
        case AP_ENTRY_REQUEST_CODE: { 
            std::pair<uint8_t, AppendEntryRequest> request;
            memcpy(&request, data, sizeof(request));
            RaftServer::addMessage(request.second); break;
        } case AP_ENTRY_RESPONSE_CODE: { 
            std::pair<uint8_t, AppendEntryResponse> response;
            memcpy(&response, data, sizeof(response));
            RaftServer::addMessage(response.second); break;
        } case RE_VOTE_REQUEST_CODE: { 
            std::pair<uint8_t, RequestVoteRequest> request;
            memcpy(&request, data, sizeof(request));
            RaftServer::addMessage(request.second); break;
        } case RE_VOTE_RESPONSE_CODE: { 
            std::pair<uint8_t, RequestVoteResponse> response;
            memcpy(&response, data, sizeof(response));
            RaftServer::addMessage(response.second); break;
        } case STORAGE_REQUEST_CODE: {
            std::pair<uint8_t, StorageRequest> response;
            memcpy(&response, data, sizeof(response));
            addMessage(response.second); break;
        } case STORAGE_RESPONSE_CODE: {
            std::pair<uint8_t, StorageResponse> response;
            memcpy(&response, data, sizeof(response));
            addMessage(response.second); break;
        } default: break; // Invalid code
    }
}

void Node::begin(long long currentTime, MACAddress serverID, const std::vector<MACAddress>& otherMembers) {
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {  } // Something went wrong
    
    // register all peers
    for (int i = 0; i < otherMembers.size(); i++) {
        esp_now_peer_info peer;
        memcpy(peer.peer_addr, &(otherMembers[i].address), 6);
        peer.channel = 0; // This can be changed depending on what is busy in the network
        peer.encrypt = false;

        if (esp_now_add_peer(&peer) != ESP_OK) { } // something else went wrong
    }   

#ifndef IS_SIMULATION // If this is a simulation, we will just call the onRecvData method directly
    static auto callbck = [this](const uint8_t* mac_addr, const uint8_t* data, int data_len) mutable -> void { onRecvData(mac_addr, data, data_len); };
    esp_now_register_recv_cb([](const uint8_t* mac_addr, const uint8_t* data, int data_len) mutable -> void { callbck(mac_addr, data, data_len); });  
#endif

    initializeRaftServer(currentTime, serverID, otherMembers);
    m_responseTimeline = currentTime;
    m_responseTimeout = currentTime;

    m_nextDataUpdateTime = currentTime;
    m_dataSecuredIndex = -1;
    m_securedData.resize(0);
    m_pendingData = nullptr;
}

void Node::update(long long currentTime) {
    // If this node is the leader, check if a new storage location is leader
    while (!m_storageRequests.empty()) {
        sendMessage(m_storageRequests.get().senderID, StorageResponse{m_serverID, true}); // TODO: actually check if this node can store data
    }

    // If leader, make sure that the storage situation is taken care of
    if (getCurrentState() == State::LEADER) {
        if (getLogSize() == 0 or (currentTime - getLogEntry(getLogSize()-1).time) >= STORAGE_RESET_DURATION) {
            // Create a new storage location
            if (m_responseTimeout <= currentTime) {
                while (!m_storageResponse.empty()) m_storageResponse.get();
                for (MACAddress addr : m_otherServers) sendMessage(addr, StorageRequest{m_serverID});
                m_responseTimeline = currentTime + TIME_TO_WAIT_FOR_STORAGE_RESPONSE;
                m_responseTimeout = currentTime + 2*TIME_TO_WAIT_FOR_STORAGE_RESPONSE;
            } else if (m_responseTimeline <= currentTime) {
                std::vector<MACAddress> storables;
                while (!m_storageResponse.empty()) {
                    StorageResponse response = m_storageResponse.get();
                    if (response.canStore) storables.emplace_back(response.senderID);
                }
                if (storables.size() > 0) {
                    std::uniform_int_distribution<int> distribution(0,storables.size() - 1);
                    addLogEntry({getCurrentTerm(), currentTime, storables[distribution(generator)]});
                }
            }
        }
    }

    // If needed, update data stored on this node.
    if (currentTime >= m_nextDataUpdateTime) {
        m_nextDataUpdateTime = currentTime + DATA_UPDATE_PULSE;
        DataLinkedList* current = m_pendingData;
        while (current != nullptr) {
            // do something with this
            current = current->next;
        }
    }

    updateRaftServer(currentTime);
}