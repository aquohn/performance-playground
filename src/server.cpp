#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <sstream>

#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "cache.hpp"
#include "server.hpp"
#include "utils.hpp"

#define MAX_LISTEN_BACKLOG 4096
#define EPOLL_TIMEOUT_MS -1

in_port_t port;

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
    close(sockfd);
    pp::fatal_error("Socket bind failed: {}\n", strerror(errno));
  }
  if (listen(sockfd, MAX_LISTEN_BACKLOG) < 0) {
    close(sockfd);
    pp::fatal_error("Socket listen failed: {}\n", strerror(errno));
  }

  struct epoll_event ev;
  int epollfd = epoll_create1(0), nev = 1;
  if (epollfd < 0) {
    pp::fatal_error("epoll fd construction failed: {}\n", strerror(errno));
  }
  ev.events = EPOLLIN;
  ev.data.fd = sockfd;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);

  std::vector<struct epoll_event> evqueue;
  evqueue.resize(1);
  while (true) {
    // TODO main loop
    // TODO adjust maxevents
    int epoll_cnt = epoll_wait(epollfd, evqueue.data(), nev, EPOLL_TIMEOUT_MS);
    if (epoll_cnt < 0) {
      close(sockfd);
      pp::fatal_error("epoll wait failed: {}\n", strerror(errno));
    }

    for (int i = 0; i < epoll_cnt; ++i) {
      if (evqueue[i].data.fd != sockfd) {
        // read and send all data TODO
        continue;
      }

      // discard connector info
      int connfd = accept(sockfd, NULL, NULL);
      if (connfd < 0) {
        close(sockfd);
        pp::fatal_error("epoll wait failed: {}\n", strerror(errno));
      }
    }
    break;
  }
}
