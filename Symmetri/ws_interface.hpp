#pragma once
#include "model.h"
#include <iostream>
#include <seasocks/PrintfLogger.h>
#include <seasocks/Server.h>

struct Output : seasocks::WebSocket::Handler {
  std::set<seasocks::WebSocket *> connections;
  void onConnect(seasocks::WebSocket *socket) override { connections.insert(socket); }
  void onDisconnect(seasocks::WebSocket *socket) override { connections.erase(socket); }
  void send(const std::string &msg) {
    for (auto *con : connections) {
      con->send(msg);
    }
  }
};
