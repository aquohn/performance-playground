#include "utils.hpp"

#include <chrono>
#include <string>
#include <unordered_set>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

struct Index {
  fs::path dpath;
  std::vector<std::string> ids;
  std::unordered_set<std::string> id_set;

  Index();
  Index(fs::path srv);
};

struct Record {
  size_t incorrect = 0, real = 0;
  std::vector<std::chrono::duration<double, std::milli>> rtts;
};

struct ClientConfig {
  double sleep_avg_ms, sleep_stddev_ms;
  int reqs, reps;
  Index index;
  struct sockaddr_in server_addr;

  ClientConfig(int argc, char *argv[]);
};

