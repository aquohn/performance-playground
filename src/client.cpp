#include <sstream>
#include <fstream>
#include <random>
#include <cstdio>
#include <cstring>
#include <print>
#include <thread>
#include <chrono>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "client.hpp"

class Requestor {
  private:
    Index const * index;
    struct sockaddr_in const * server_addr;

    std::mt19937 prng;
    int sockfd;

    int connect_to_server() {
      // TODO error handling
      sockfd = socket(AF_INET, SOCK_STREAM, 0);
      return connect(sockfd, (struct sockaddr *) server_addr, sizeof(*server_addr));
    }

    void request_exist() {
      std::uniform_int_distribution<size_t> distr(0, index->ids.size() - 1);
      request_exist(index->ids[distr(prng)]);
    }

    void request_exist(std::string id) {
      // choose a random id
      fs::path fpath; bool ok = false;
      for (auto const& f : fs::directory_iterator(index->dpath)) {
        if (f.path().extension() == "pdf"s) {
          fpath = f; ok = true; break;
        }
      }
      if (!ok) return;

      size_t fsz = fs::file_size(fpath);
      std::vector<char> buf(fsz + 1);
      std::ifstream fstrm(fpath.c_str(), std::ios::binary);
      fstrm.read(buf.data(), fsz);
      buf[fsz] = 0;
      if (!fstrm) return;
      hash_t fhash = sha256(buf);

      auto startt = std::chrono::steady_clock::now();
      send(sockfd, id.c_str(), id.size(), 0);
      size_t rsz = read(sockfd, buf.data(), fsz);
      auto endt = std::chrono::steady_clock::now();
      hash_t rhash = sha256(buf);

      // TODO keep statistics
    }

    void request_rand() {
      std::vector<char> idvec(8);
      std::string id;

      std::uniform_int_distribution<size_t> distr(0, 35);
      for (;;) {
        for (int i = 0; i < 8; ++i) {
          char c = distr(prng);
          if (c < 10) {
            c += '0';
          } else {
            c += 'A' - 10;
          }
          idvec[i] = c;
        }
        id = std::string(idvec.begin(), idvec.end());
        if (index->id_set.find(id) == index->id_set.end()) {
          continue;
        }
        break;
      }

      std::vector<char> buf(1024);
      send(sockfd, id.c_str(), id.size(), 0);
      size_t rsz = read(sockfd, buf.data(), 1024 - 1);

      // TODO keep statistics
    }
  public:
    Requestor(Index const * _index, struct sockaddr_in const * _addr) : index(_index), server_addr(_addr) {
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

void request_thread(ClientConfig config, Index const * index, struct sockaddr_in const * server_addr) {
  Requestor R(index, server_addr);
  std::random_device rd;
  std::mt19937 prng = std::mt19937(rd());

  int rcount = 0;
  while (rcount != config.reps) {
    R.request();
    ++rcount;

    double sleep_ms = std::normal_distribution(config.sleep_avg_ms, config.sleep_stddev_ms)(prng);
    sleep_ms = (sleep_ms > 0) ? sleep_ms : config.sleep_avg_ms - sleep_ms;
    std::this_thread::sleep_for(std::chrono::duration<double, std::milli>{sleep_ms});
  }
}

int main(int argc, char *argv[]) {
  // TODO construct threads for requestors
  ClientConfig config; Index index;
  config.sleep_avg_ms = 100., config.sleep_stddev_ms = 50.;
  struct sockaddr_in addr;
  int reqs, ret;

	if (argc != 5) {
		std::print(stderr, "Usage: {} <server addr:port> <content directory> <requestors> <reps>", argv[0]);
		exit(1);
	}

  addr.sin_family = AF_INET;
	std::string server_str(argv[1]);
  auto delim_it = std::next(server_str.begin(), server_str.find(':'));
  ret = inet_pton(AF_INET, std::string(server_str.begin(), delim_it).data(), &addr.sin_addr);
  if (ret == 0) {
    std::print(stderr, "Invalid address.");
    exit(1);
  } else if (ret < 0) {
    std::print(stderr, "Address parsing failed with {}", strerror(errno));
    exit(1);
  }
  ++delim_it;
  std::stringstream port_stream(std::string(delim_it, server_str.end()));
  port_stream >> addr.sin_port;
  if (port_stream.bad() || port_stream.fail()) {
    std::print(stderr, "Port parsing failed.");
    exit(1);
  }

	std::filesystem::path srv(argv[2]);
	// TODO error handling

	std::stringstream req_stream(argv[3]);
	req_stream >> reqs;
	// TODO error handling

	std::stringstream rep_stream(argv[4]);
	rep_stream >> config.reps;
	// TODO error handling

  std::vector<std::jthread> threads;
  for (int i = 0; i < reqs; ++i) {
    // TODO extract statistics on SIGINT or join
    threads.emplace_back(request_thread, config, &index, &addr);
  }

  return 0;
}
