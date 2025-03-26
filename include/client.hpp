#include "utils.hpp"

struct Index {
  fs::path dpath;
  std::vector<std::string> ids;
  std::unordered_set<std::string> id_set;
};

struct ClientConfig {
  double sleep_avg_ms, sleep_stddev_ms;
  int reps;
};

