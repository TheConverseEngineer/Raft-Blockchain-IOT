#ifndef SERIAL_TESTER_H
#define SERIAL_TESTER_H

#include <vector>
#include <string>
#include "raft.h"

void runCommandSwitch(RaftServer& server);

void sendUpdate(RaftServer& server);

void sendVoteResult(RaftServer& server);

void sendVoteRequest(RaftServer& server);

void sendAppendResult(RaftServer& server);

void sendAppendRequest(RaftServer& server);

void addLogEntry(RaftServer& server);

#endif