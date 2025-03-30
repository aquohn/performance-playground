#include <algorithm>
#include <atomic>
#include <format>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <thread>

#include <csignal>
#include <cstdio>
#include <cstring>

#include "client.hpp"

#define SERVER_RESP_SZ 1

Index::Index(fs::path srv) {
  fs::directory_entry srv_ent(srv);
  if (!srv_ent.is_directory()) {
    std::cerr << "Data directory path is not a directory.\n";
    exit(1);
  }
  for (auto const &f : fs::directory_iterator(srv)) {
    std::string fname = f.path().string();
    if (fname[0] == '.')
      continue;
    ids.push_back(fname);
    id_set.insert(fname);
  }
}

ClientConfig::ClientConfig(char *argv[]) : index(Index(fs::path(argv[2]))) {
  // TODO getopt for this
  sleep_avg_ms = 100.;
  sleep_stddev_ms = 50.;
  server_addr.sin_family = AF_INET;
  int ret;

  std::string server_str(argv[1]);
  auto delim_it = std::next(server_str.begin(), server_str.find(':'));
  ret = inet_pton(AF_INET, std::string(server_str.begin(), delim_it).data(),
                  &server_addr.sin_addr);
  if (ret == 0) {
    std::cerr << std::format("Invalid address.\n");
    exit(1);
  } else if (ret < 0) {
    std::cerr << std::format("Address parsing failed: {}\n", strerror(errno));
    exit(1);
  }
  ++delim_it;
  std::stringstream port_stream(std::string(delim_it, server_str.end()));
  port_stream >> server_addr.sin_port;
  if (port_stream.bad() || port_stream.fail()) {
    std::cerr << std::format("Port parsing failed.\n");
    exit(1);
  }

  std::stringstream req_stream(argv[3]);
  req_stream >> reqs;
  if (req_stream.bad() || req_stream.fail()) {
    std::cerr << std::format("Requestor count parsing failed.\n");
    exit(1);
  }

  std::stringstream rep_stream(argv[4]);
  rep_stream >> reps;
  if (rep_stream.bad() || rep_stream.fail()) {
    std::cerr << std::format("Repetition count parsing failed.\n");
    exit(1);
  }
}

static std::atomic<int> cleanup = 0;

class Requestor {
private:
  Record *record;
  Index const *index;
  struct sockaddr_in const *server_addr;

  std::mt19937 prng;
  int sockfd = -1;

  int connect_to_server() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      std::cerr << std::format("Socket construction failed: {}\n",
                               strerror(errno));
      exit(1);
    }
    // TODO error handling
    return connect(sockfd, (struct sockaddr *)server_addr,
                   sizeof(*server_addr));
  }

  void close_connection() {
    if (sockfd >= 0)
      close(sockfd);
  }

  void request_exist() {
    // choose a random id
    std::uniform_int_distribution<size_t> distr(0, index->ids.size() - 1);
    request_exist(index->ids[distr(prng)]);
  }

  void request_exist(std::string id) {
    fs::path fpath;
    bool ok = false;
    for (auto const &f : fs::directory_iterator(index->dpath / id)) {
      if (f.path().extension() == "pdf"s) {
        fpath = f;
        ok = true;
        break;
      }
    }
    if (!ok)
      return;

    size_t fsz = fs::file_size(fpath);
    std::vector<char> buf(fsz + 1);
    std::ifstream fstrm(fpath.c_str(), std::ios::binary);
    fstrm.read(buf.data(), fsz);
    buf[fsz] = 0;
    if (!fstrm)
      return;
    hash_t fhash = sha256(buf);

    ++(record->real);
    auto startt = std::chrono::steady_clock::now();
    send(sockfd, id.c_str(), id.size(), 0);
    long rsz = read(sockfd, buf.data(), fsz);
    auto endt = std::chrono::steady_clock::now();
    hash_t rhash = sha256(buf);

    if (rsz != (long) fsz || fhash != rhash)
      ++(record->incorrect);
    record->rtts.push_back(endt - startt);
  }

  void request_rand() {
    std::vector<char> idvec(8);
    std::string id;

    std::uniform_int_distribution<size_t> distr(0, 35);
    while (true) {
      for (int i = 0; i < 8; ++i) {
        char c = distr(prng);
        c += ((c < 10) ? '0' : 'A' - 10);
        idvec[i] = c;
      }
      id = std::string(idvec.begin(), idvec.end());
      if (index->id_set.find(id) == index->id_set.end()) {
        continue;
      }
      break;
    }

    std::vector<char> buf(SERVER_RESP_SZ + 1);
    auto startt = std::chrono::steady_clock::now();
    send(sockfd, id.c_str(), id.size(), 0);
    [[maybe_unused]] long rsz = read(sockfd, buf.data(), SERVER_RESP_SZ);
    auto endt = std::chrono::steady_clock::now();
    record->rtts.push_back(endt - startt);

    // TODO check that correct response is returned
  }

public:
  Requestor(Record *_record, Index const *_index,
            struct sockaddr_in const *_addr)
      : record(_record), index(_index), server_addr(_addr) {
    std::random_device rd;
    prng = std::mt19937(rd());
  }
  void request(float p = 0.) {
    float res = std::uniform_real_distribution(0., 1.)(prng);
    if (res < p) {
      request_rand();
    } else {
      request_exist();
    }
  }
};

void request_thread(ClientConfig config, Record *record) {
  std::signal(SIGINT, SIG_IGN);
  Requestor R(record, &(config.index), &(config.server_addr));
  std::random_device rd;
  std::mt19937 prng = std::mt19937(rd());

  int rcount = 0;
  while (!cleanup.load() && rcount != config.reps) {
    R.request();
    ++rcount;

    double sleep_ms = std::normal_distribution(config.sleep_avg_ms,
                                               config.sleep_stddev_ms)(prng);
    sleep_ms = (sleep_ms > 0) ? sleep_ms : config.sleep_avg_ms - sleep_ms;
    std::this_thread::sleep_for(
        std::chrono::duration<double, std::milli>{sleep_ms});
  }
}

void notify_cleanup(int signum) { cleanup.store(1); }

int main(int argc, char *argv[]) {
  if (argc != 5) {
    std::cerr << std::format("Usage: {} <server addr:port> <content directory> "
                             "<requestor count> <reps>\n",
                             argv[0]);
    exit(1);
  }
  ClientConfig config(argv);

  std::signal(SIGINT, notify_cleanup);
  std::vector<std::jthread> threads;
  threads.reserve(config.reqs);
  std::vector<Record> records(config.reqs);
  for (int i = 0; i < config.reqs; ++i) {
    threads.emplace_back(request_thread, config, &(records[i]));
  }

  for (int i = 0; i < config.reqs; ++i) {
    threads[i].join();
  }
  std::vector<double> rtts;
  long long total = 0, errors = 0;
  for (auto rec : records) {
    total += rec.real;
    errors += rec.incorrect;
    for (auto rtt : rec.rtts) {
      rtts.push_back(rtt / 1.0ms);
    }
  }
  double rtt_avg =
      std::accumulate(rtts.begin(), rtts.end(), 0) / ((double)rtts.size());
  std::for_each(rtts.begin(), rtts.end(),
                [=](const double x) { return pow(x - rtt_avg, 2); });
  double rtt_stddev =
      std::accumulate(rtts.begin(), rtts.end(), 0) / (rtts.size() - 1.5);

  std::cout << std::format("RTT {:2f} +/- {:2f} ms\tErrors {} / {} ({:1f}%)\n",
                           rtt_avg, rtt_stddev, total, errors,
                           (double)total / errors);

  return 0;
}
