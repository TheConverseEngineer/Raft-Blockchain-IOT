#define IS_SIMULATION

#include "node.h"
#include <map>
#include <utility>
#include <thread>
#include <chrono>
#include <string>
#include <fstream>
#include <atomic>

#include "logcat.h"

bool ext_withTimestamp;
long long (*ext_pFunc)();

int N = 6;
std::vector<MACAddress> mac_list;
std::map<MACAddress, Node*> nodes;

std::chrono::steady_clock::time_point startTime;
long long getCurrentTime() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count(); }

bool simulatorIsRunning = true;
bool pauseForReport = false;

long long END_TIME = 10000;

long long updateFrequency;

/** Simple AtomicBoolWrapper to allow for vectors
 * 
 * Based on a StackOverflow post by jogojapan: https://stackoverflow.com/questions/13193484/how-to-declare-a-vector-of-atomic-in-c (7/5/2023)
*/
struct AtomicBoolWrapper {
  std::atomic_bool value;
  AtomicBoolWrapper(): value() {}
  AtomicBoolWrapper(const std::atomic_bool &_value): value(_value.load()) {}
  AtomicBoolWrapper(const AtomicBoolWrapper &_value): value(_value.value.load()) {}
  AtomicBoolWrapper& operator=(const AtomicBoolWrapper &_value) { value.store(_value.value.load()); }
};

std::vector<AtomicBoolWrapper> isEnabled;
std::vector<std::thread> threads(0);
int disabledNodes = 0;

double disableRate;

/** Links communication between simulated nodes */
int esp_now_send(const uint8_t *peer_addr, const uint8_t *data, std::size_t len) { 
    
    MACAddress addr;
    memcpy(&addr, peer_addr, sizeof(addr));    
    if (nodes.count(addr)) {
        nodes[addr]->onRecvData(peer_addr, data, len);
        //Log::log("MASTER", "SENT COMMUNICATION TO", addr);
    } else std::cout << "MESSAGE FAILED! (Non-Existant Node)\n";

    return -1;
}

/** Creates all servers (does not create threads)*/
inline void createServers() {
    isEnabled.resize(N, {true});
    for (int i = 0; i < N; i++) mac_list.emplace_back(MACAddress(i,1,1,1,1,1));
    for (int i = 0; i < N; i++) {
        std::vector<MACAddress> otherAddresses;
        for (int j = 0; j < N-1; j++) otherAddresses.emplace_back(mac_list[j + ((j>=i)?1:0)]);
        
        nodes.insert(std::make_pair(mac_list[i], new Node()));
        nodes[mac_list[i]]->begin(0, mac_list[i], otherAddresses);
    }
}

/** Creates and activates all threads */
inline void launchThreads() {
    for (int i = 0; i < N; i++) {
        threads.push_back(std::thread([i](Node *target_node) -> void {
            while (simulatorIsRunning) {
                while (pauseForReport);
                while (!isEnabled[i].value);
                target_node->update(getCurrentTime());
                std::this_thread::sleep_for(std::chrono::milliseconds(9));
            }
        }, nodes[mac_list[i]]));
    }
}

int main(int argc, char *argv[]) {

    if (argc != 5) {
        std::cout << "FATAL ERROR: Wrong number of parameters\nExpected useage: <file>.exe server_count duration update_frequency disable_rate(rec: .00004)";
        return -1;
    }

    // Store command-line arguments
    N = std::stoi(argv[1]);
    END_TIME = std::stoll(argv[2]);
    updateFrequency = std::stoll(argv[3]);
    disableRate = std::stod(argv[4]);
    std::cout << "Activating Simulation\n\tNumber of nodes: " << N << "\n\tSimulation Duration (ms): " << END_TIME << "\n\tUpdate Frequency: " << updateFrequency << "\n";

    // Set up logcat
    ext_pFunc = getCurrentTime;
    ext_withTimestamp = true;

    // Create all of the servers
    createServers();

    // Launch threads
    std::cout << "[MASTER] ALL SERVERS INITIATED - LAUNCHING THREADS";
    launchThreads();
    std::cout << "[MASTER] ALL THREADS LAUNCHED";

    // Set up timers
    startTime = std::chrono::steady_clock::now();
    long long nextUpdateDue = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
    long long nextTimestampForError = nextUpdateDue;

    // Simulation update loop
    do {
        // Decide if a node should be shut down
        if (getCurrentTime() >= nextTimestampForError) {
            nextTimestampForError++;
            double random_val = ((double)rand())/RAND_MAX;
            if (random_val <= disableRate) {
                if (disabledNodes*2 >= N) { // Have to enable
                    std::vector<int> disabledNodeIndexes;
                    for (int i = 0; i < N; i++) if (!isEnabled[i].value) disabledNodeIndexes.emplace_back(i);
                    int selectedNode = (((long long)disabledNodes)*rand())/RAND_MAX;
                    Log::log("MASTER", "Enabling node", mac_list[disabledNodeIndexes[selectedNode]]);
                    isEnabled[disabledNodeIndexes[selectedNode]].value = true;
                    disabledNodes = disabledNodeIndexes.size() - 1;
                } else if (disabledNodes <= 0)  { // Have to disable
                    int selectedNode = (((long long)N)*rand())/RAND_MAX;
                    Log::log("MASTER", "Disabling node", mac_list[selectedNode]);
                    isEnabled[selectedNode].value = false;
                    disabledNodes = 1;
                } else { // Do either (random)
                    bool shouldDisable = (((double)rand())/RAND_MAX) >= 0.5;
                    std::vector<int> targetedNodes;
                    for (int i = 0; i < N; i++) if ((isEnabled[i].value and shouldDisable) or (!isEnabled[i].value and !shouldDisable)) targetedNodes.emplace_back(i);
                    int selectedNode = (((long long)targetedNodes.size())*rand())/RAND_MAX;
                    Log::log("MASTER", (shouldDisable?"Disabling node":"Enabling node"), mac_list[targetedNodes[selectedNode]]);
                    isEnabled[targetedNodes[selectedNode]].value = !shouldDisable;
                    disabledNodes += (shouldDisable?1:-1);
                }
            }
        }
        

        // Do a log post if needed
        if (getCurrentTime() >= nextUpdateDue) {
            pauseForReport = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(2)); // Wait for any partial exections to halt
            Log::log("MASTER", "BEGINNING UPDATE AT TIME", getCurrentTime());
            for (int i = 0; i < N; i++) {
                Log::log(mac_list[i], "Is active", isEnabled[i].value?"Yes":"No");
                Log::log(mac_list[i], "Current state", stateToStr(nodes[mac_list[i]]->getCurrentState()));
                Log::log(mac_list[i], "Log size", nodes[mac_list[i]]->getLogSize());
                Log::log(mac_list[i], "Commit index", nodes[mac_list[i]]->getCommitIndex());
                Log::log(mac_list[i], "Current term", nodes[mac_list[i]]->getCurrentTerm());
                if (isEnabled[i].value && nodes[mac_list[i]]->getCurrentState()==State::LEADER && nodes[mac_list[i]]->getLogSize() > 0) {
                    Log::log(mac_list[i], "Last log entry", nodes[mac_list[i]]->getLogEntry(nodes[mac_list[i]]->getLogSize()-1));
                }
            }
            pauseForReport = false;
            nextUpdateDue = getCurrentTime() + updateFrequency;
        }
    } while (getCurrentTime() < END_TIME);

    simulatorIsRunning = false;
    pauseForReport = false;
    for (int i = 0; i < N; i++) isEnabled[i].value = true;

    for (std::thread& i : threads) i.join();

    Log::log("MASTER", "ALL THREADS TERMINATED");
}