#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <sstream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "server.hpp"
#include "loop.hpp"
#include "cache.hpp"
#include "utils.hpp"

in_port_t port;

// TODO create hierarchical templated switches for choosing components

int main(int argc, char *argv[]) {
  if (argc != 3) {
    pp::fatal_error("Usage: {} <port> <content directory>\n", argv[0]);
  }

  std::stringstream port_stream(argv[1]);
  port_stream >> port;
  if (port_stream.bad() || port_stream.fail()) {
    pp::fatal_error("Port parsing failed.\n");
  }

  fs::path srv(argv[2]);
  fs::directory_entry srv_ent(srv);
  if (!srv_ent.is_directory()) {
    pp::fatal_error("Data directory path is not a directory.\n");
  }

  struct sockaddr_in addr{.sin_family = AF_INET,
                          .sin_port = port,
                          .sin_addr = {.s_addr = INADDR_ANY}};
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    pp::fatal_error("Socket construction failed: {}\n", strerror(errno));
  }
  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    pp::fatal_error("Socket bind failed: {}\n", strerror(errno));
  }
  if (listen(sockfd, MAX_LISTEN_BACKLOG) < 0) {
    pp::fatal_error("Socket listen failed: {}\n", strerror(errno));
  }

  // TODO call appropriately templated loop

}
