#include "serial_tester.h"
#include <Arduino.h>

void runCommandSwitch(RaftServer& server) {
  Serial.print("Please enter command: ");
  while (Serial.available() <= 0) { }
  char data = Serial.read();
  Serial.print(data);
  switch (data) {
    case 'a': Serial.println(" => Updating device"); sendUpdate(server); return;
    case 'b': Serial.println(" => Sending vote result"); sendVoteResult(server); return;
    case 'c': Serial.println(" => Sending vote request"); sendVoteRequest(server); return;
    case 'd': Serial.println(" => Sending append request"); sendAppendRequest(server); return;
    case 'e': Serial.println(" => Sending append result"); sendAppendResult(server); return;
    case 'f': Serial.println(" => Adding log entry"); addLogEntry(server); return;
    default: Serial.println(" => Not Found"); return;
  }
}

std::vector<int> requestData(const std::vector<std::string> params) {
    std::vector<int> results(params.size());
    for (int i = 0; i < params.size(); i++){
        Serial.print("\t"); Serial.print(params[i].c_str()); Serial.print(": ");
        while (Serial.available() <= 0) { }
        char data = Serial.read();
        Serial.println(data);
        results[i] = data-'0';
    }
    return results;
}

void sendUpdate(RaftServer& server) {
    Serial.print("\tTime (4 digits): ");
    while (Serial.available() < 4) { }
    char a[4];
    for (int i = 0; i < 4; i++) a[i] = Serial.read();
    long long time = (a[0]-'0')*1000 + (a[1]-'0')*100 + (a[2]-'0')*10 + (a[3]-'0');
    Serial.println(time);
    server.update(time);
}

void sendVoteResult(RaftServer& server) {
  std::vector<int> data = requestData(std::vector<std::string>{"Term", "Voted (0=no, 1=yes)"});
  server.addMessage(RequestVoteResponse{data[0], data[1]!=0});
}

void sendVoteRequest(RaftServer& server) {
  std::vector<int> data = requestData(std::vector<std::string>{"Term", "Cand. ID", "Last log index", "Last log term"});
  server.addMessage(RequestVoteRequest{data[0], data[1], data[2], data[3]});
}

void sendAppendResult(RaftServer& server) {
  std::vector<int> data = requestData(std::vector<std::string>{"Term", "Sender ID", "Successful (0=no, 1=yes)"});
  server.addMessage(AppendEntryResponse{data[0], data[1], data[2]!=0});
}

void sendAppendRequest(RaftServer& server) {
  std::vector<int> data = requestData(std::vector<std::string>{"Term", "Prev. log index", "Prev. log term", "Number of entries", "Term of entries", "Leader commit index"});
  server.addMessage(AppendEntryRequest{data[0], data[1], data[2], data[3], std::vector<Entry>(data[3], {data[4]}), data[5]});

}


void addLogEntry(RaftServer& server) {
  std::vector<int> data = requestData(std::vector<std::string>{"Term"});
  server.addLogEntry(Entry{data[0]});
}

