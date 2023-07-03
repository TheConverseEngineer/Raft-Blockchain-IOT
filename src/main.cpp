/*********
  Rui Santos
*********/

#include <Arduino.h>

#include <dht.h>

dht DHT;

#define DHT11_PIN 5

void setup(){
  Serial.begin(115200);
}

void loop(){
  int chk = DHT.read11(DHT11_PIN);
  Serial.print("Temperature = ");
  Serial.println(DHT.temperature);
  Serial.print("Humidity = ");
  Serial.println(DHT.humidity);
  delay(2000);
}

/*
#include "sha256.h"
#include "serial_tester.h"

RaftServer server;

long long RaftServer::getElectionTimeout() { return 250LL; }
long long RaftServer::getHeartbeatTimeout() { return 50LL; }

void RaftServer::sendMessage(int recipient, const RequestVoteRequest& msg) { Serial.print("Server 1 send a vote request to server "); Serial.println(recipient); }
void RaftServer::sendMessage(int recipient, const AppendEntryRequest& msg) { Serial.print("Server 1 send a append request to server "); Serial.println(recipient); }
void RaftServer::sendMessage(int recipient, const RequestVoteResponse& msg) { Serial.print("Server 1 send a vote responce to server "); Serial.println(recipient); }
void RaftServer::sendMessage(int recipient, const AppendEntryResponse& msg) { Serial.print("Server 1 send a append responce to server "); Serial.println(recipient); }

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(5, OUTPUT);
  server.initialize(0LL, 1, std::vector<int>{2, 3, 4, 5});
}

void loop() {
  // runCommandSwitch(server);
  digitalWrite(5, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(5, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);

}



Hash useage:

Hash hash = hash_data("hello world");
char hexstring[65];  // needs to be at least 64 hex digits + 1 for the null terminator
sprintf(hexstring, "%016llx%016llx%016llx%016llx", hash.a, hash.b, hash.c, hash.d);
Serial.println(hexstring);
*/
