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

#include "cache.hpp"
#include "loop.hpp"
#include "server.hpp"
#include "utils.hpp"

#define MAP_TEMPLATE template <typename, typename> typename
#define CACHE_TEMPLATE template <MAP_TEMPLATE, typename> typename
#define LOOP_TEMPLATE template <MAP_TEMPLATE, typename> typename

static constexpr char usage[] =
    "Usage: {} -l <loop> -r <request map> -c <cache eviction> -m <cache map> "
    "-f <file backend> [ -n cache capacity ] <port> <content directory>\n"
    "supported -l: epoll\n"
    "supported -r: umap\n"
    "supported -c: none, mutex\n"
    "supported -m: umap\n"
    "supported -f: filesystem\n";

#define SINGLE_ARG(...) __VA_ARGS__
#define USE_BACKEND(params, name, key, backend)                                \
  if (config.name == key) {                                                    \
    playground<params backend> play;                                           \
    loop(play, config, sockfd);                                                \
  }
#define EXIT_MISSING_BACKEND(name, Desc)                                       \
  {                                                                            \
    if (config.name.size() == 0) {                                             \
      pp::log_error(usage, "<program name>");                                  \
      pp::fatal_error(#Desc " implementation not specified.\n");               \
    } else {                                                                   \
      pp::log_error(usage, "<program name>");                                  \
      pp::fatal_error(#Desc " implementation {} unknown\n", config.name);      \
    }                                                                          \
  }

struct ServerConfig {
  std::string loop, rmap, cache, cmap, filebe;
  int cache_capacity = 100;
  in_port_t port;
  fs::path srv;

  ServerConfig(int argc, char **argv);
};

ServerConfig::ServerConfig(int argc, char **argv) {
  int opt;
  std::stringstream ss;
  while ((opt = getopt(argc, argv, "l:r:c:m:f:n:")) != -1) {
    switch (opt) {
    case 'l':
      loop.assign(optarg);
      break;
    case 'r':
      rmap.assign(optarg);
      break;
    case 'c':
      cache.assign(optarg);
      break;
    case 'm':
      cmap.assign(optarg);
      break;
    case 'f':
      filebe.assign(optarg);
      break;
    case 'n':
      ss.str(optarg);
      ss >> cache_capacity;
      break;
    default:
      pp::fatal_error(usage, argv[0]);
    }
    if (ss.bad() || ss.fail()) {
      pp::fatal_error("Parsing -{} failed.\n", opt);
    }
    ss.clear();
  }

  if (optind + 2 > argc) {
    pp::log_error("Insufficient arguments provided (expected 2, got {}).\n",
                  argc - optind);
    pp::fatal_error(usage, argv[0]);
  }

  std::stringstream port_stream(argv[optind++]);
  port_stream >> port;
  if (port_stream.bad() || port_stream.fail()) {
    pp::fatal_error("Port parsing failed.\n");
  }

  srv.assign(argv[optind++]);
  fs::directory_entry srv_ent(srv);
  if (!srv_ent.is_directory()) {
    pp::fatal_error("Data directory path is not a directory.\n");
  }
}

template <typename, typename> struct VoidMap {};
template <MAP_TEMPLATE, typename> struct VoidCache {};
template <MAP_TEMPLATE, typename> struct VoidLoop {};

// TODO: can we do without this type and just specialise a function?
// A base type with template <typename Args...> doesn't work because of the
// template template parameters
template <LOOP_TEMPLATE Loop = VoidLoop, MAP_TEMPLATE RMap = VoidMap,
          CACHE_TEMPLATE Cache = VoidCache, MAP_TEMPLATE CMap = VoidMap,
          typename Backend = void>
struct playground {};

template <LOOP_TEMPLATE Loop, MAP_TEMPLATE RMap, CACHE_TEMPLATE Cache,
          MAP_TEMPLATE CMap, typename Backend>
void loop(playground<Loop, RMap, Cache, CMap, Backend> play,
          const ServerConfig &config, int sockfd) {
  Loop<RMap, Cache<CMap, Backend>> main_loop(config.srv, config.cache_capacity);
  main_loop.loop(sockfd);
}
template <LOOP_TEMPLATE Loop, MAP_TEMPLATE RMap, CACHE_TEMPLATE Cache,
          MAP_TEMPLATE CMap>
void loop(playground<Loop, RMap, Cache, CMap> play, const ServerConfig &config,
          int sockfd) {
  USE_BACKEND(SINGLE_ARG(Loop, RMap, Cache, CMap, ), filebe, "filesystem",
              FSBackend)
  else EXIT_MISSING_BACKEND(filebe, File backend)
}
template <LOOP_TEMPLATE Loop, MAP_TEMPLATE RMap, CACHE_TEMPLATE Cache>
void loop(playground<Loop, RMap, Cache> play, const ServerConfig &config,
          int sockfd) {
  USE_BACKEND(SINGLE_ARG(Loop, RMap, Cache, ), cmap, "umap", UMap)
  else EXIT_MISSING_BACKEND(rmap, Cache mapping)
}
template <LOOP_TEMPLATE Loop, MAP_TEMPLATE RMap>
void loop(playground<Loop, RMap> play, const ServerConfig &config, int sockfd) {
  USE_BACKEND(SINGLE_ARG(Loop, RMap, ), cmap, "mutex", MutexCache)
  else USE_BACKEND(SINGLE_ARG(Loop, RMap, ), cmap, "none", BaseCache)
  else EXIT_MISSING_BACKEND(cache, Cache eviction)
}
template <LOOP_TEMPLATE Loop>
void loop(playground<Loop> play, const ServerConfig &config, int sockfd) {
  USE_BACKEND(SINGLE_ARG(Loop, ), rmap, "umap", UMap)
  else EXIT_MISSING_BACKEND(rmap, Request mapping)
}
void loop(const ServerConfig &config, int sockfd) {
  USE_BACKEND(, loop, "epoll", EpollLoop)
  else EXIT_MISSING_BACKEND(loop, Loop)
}

int main(int argc, char *argv[]) {
  ServerConfig config(argc, argv);
  struct sockaddr_in addr{.sin_family = AF_INET,
                          .sin_port = config.port,
                          .sin_addr = {.s_addr = INADDR_ANY}};
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    pp::fatal_error("Socket construction failed: {}\n", strerror(errno));
  }
#ifdef DEBUG
  int sockoptval = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &sockoptval,
                 sizeof(sockoptval)) < 0) {
    pp::fatal_error("setsockopt failed: {}\n", strerror(errno));
  }
#endif
  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    pp::fatal_error("Socket bind failed: {}\n", strerror(errno));
  }
  if (listen(sockfd, MAX_LISTEN_BACKLOG) < 0) {
    pp::fatal_error("Socket listen failed: {}\n", strerror(errno));
  }

  loop(config, sockfd);
}
