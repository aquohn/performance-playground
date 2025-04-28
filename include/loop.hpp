#pragma once

#include <cstdlib>

#include <cstring>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "cache.hpp"
#include "utils.hpp"

#define DOC_ID_BUFLEN 32
#define MAX_LISTEN_BACKLOG 4096
#define EPOLL_TIMEOUT_MS -1

#define LOOP_TEMPLATE                                                          \
  template <template <typename, typename> typename Map, typename Cache>        \
    requires HashMap<Map, int, std::vector<char>> && DocCache<Cache>

LOOP_TEMPLATE
struct EpollLoop {
  struct epoll_event ev;
  int epollfd, nev, sockfd;
  Cache cache;

  EpollLoop(fs::path &srv, unsigned int ndocs) : cache(srv, ndocs) {}
  void loop(int _sockfd);

private:
  void close_conn(int fd) {
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev) < 0) {
      pp::fatal_error("removing client from epoll failed: {}\n", strerror(errno));
    }
    --nev;
    close(fd);
  }
};

LOOP_TEMPLATE
void EpollLoop<Map, Cache>::loop(int _sockfd) {
  sockfd = _sockfd;
  epollfd = epoll_create1(0), nev = 1;
  if (epollfd < 0) {
    pp::fatal_error("epoll fd construction failed: {}\n", strerror(errno));
  }
  ev.events = EPOLLIN;
  ev.data.fd = sockfd;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
    pp::fatal_error("adding socket to epoll failed: {}\n", strerror(errno));
  }
  std::cout << "Listening for connections...\n";

  Map<int, std::vector<char>> reqmap;
  std::vector<struct epoll_event> evqueue;
  while (true) {
    evqueue.resize(nev);
    int epoll_cnt = epoll_wait(epollfd, evqueue.data(), nev, EPOLL_TIMEOUT_MS);
    if (epoll_cnt < 0) {
      pp::fatal_error("epoll wait failed: {}\n", strerror(errno));
    }

    for (int i = 0; i < epoll_cnt; ++i) {
      int fd = evqueue[i].data.fd;
      std::vector<char> &idbuf = reqmap[fd];

      // new client connection
      if (fd == sockfd) {
        // NULL to discard client address info
        int connfd = accept(sockfd, NULL, NULL);
        if (connfd < 0) {
          pp::fatal_error("accept connection failed: {}\n", strerror(errno));
        }

        ev.events = EPOLLIN | EPOLLOUT;
        ev.data.fd = connfd;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) < 0) {
          pp::fatal_error("adding client to epoll failed: {}\n",
                          strerror(errno));
        }
        ++nev;
        continue;
      }

      auto events = evqueue[i].events;

      // client ready for read
      if (idbuf.size() > 0 && events & EPOLLOUT) {
        ll sent = cache.send(idbuf, fd);
        std::string idstr(idbuf.begin(), idbuf.end());
        if (sent < 0) {
          write(fd, "\0", 1);
        } else {
          pp::debug("Sent {} bytes for {}\n", sent, idstr);
        }
        close_conn(fd);
        idbuf.resize(0);

        // client ready for write
      } else if (idbuf.size() == 0 && events & EPOLLIN) {
        idbuf.resize(DOC_ID_BUFLEN);
        ll readlen = read(fd, idbuf.data(), DOC_ID_BUFLEN);
        if (readlen <= 0) {
          close_conn(fd);
        }
        idbuf.resize(readlen);

      } else {
        std::string idstr(idbuf.begin(), idbuf.end());
        pp::log_error("epoll error (events: {}) on request for {}\n", events, idstr);
        close_conn(fd);
      }
    }
  }
}
