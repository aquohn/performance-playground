#pragma once

#include <cassert>
#include <concepts>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include <unistd.h>

#include "utils.hpp"
#include "backend.hpp"

// TODO make cache look at total size rather than number of docs
template <typename C>
concept DocCache = std::constructible_from<C, const fs::path &, unsigned int> &&
                   requires(C cache, const std::string &idstr, const int fd) {
                     { cache.send(idstr, fd) } -> std::convertible_to<ll>;
                   };

#define CACHE_REQ_TEMPLATE                                                         \
  template <template <typename, typename> typename Map, typename Backend>      \
    requires HashMap<Map, std::string, std::vector<char>> &&                   \
             FileBackend<Backend>

CACHE_REQ_TEMPLATE
class BaseCache {
protected:
  ull ndocs;
  Map<std::string, std::vector<char>> cache_be;
  Backend file_be;

public:
  BaseCache(const fs::path &srv, ull _ndocs) : ndocs(_ndocs), file_be(srv) {}
  ll send(const std::string &idstr, const int fd);
};

CACHE_REQ_TEMPLATE &&HashMap<Map, std::string, ull> struct MutexCache
    : protected BaseCache<Map, Backend> {
protected:
  std::jthread gcthread;
  std::mutex mtx;
  std::condition_variable cv;
  std::queue<std::string> useq;
  Map<std::string, ull> usecount;
  void gc() {
    std::unique_lock lk(mtx);
    while (true) {
      cv.wait(lk);
      while (!useq.empty()) {
        usecount[useq.back()] += 1;
        const auto &frontidstr = useq.front();
        usecount[frontidstr] -= 1;
        if (usecount[frontidstr] == 0) {
          this->cache_be.erase(frontidstr);
        }
        useq.pop();
      }
    }
  }
  void push(const std::string &idstr) { useq.push(idstr); }

public:
  ll send(const std::string &idstr, const int fd) {
    std::lock_guard lk(mtx);
    ll sent = this->BaseCache<Map, Backend>::send(idstr, fd);
    push(idstr);
    if (this->ndocs < this->cache_be.size()) {
      cv.notify_one();
    }
    return sent;
  }
  MutexCache(const fs::path &srv, ull _ndocs)
      : BaseCache<Map, Backend>(srv, _ndocs),
        gcthread(&MutexCache<Map, Backend>::gc, this) {}
};

CACHE_REQ_TEMPLATE
ll BaseCache<Map, Backend>::send(const std::string &idstr, const int fd) {
  if (cache_be.contains(idstr)) {
    std::vector<char> &buf = cache_be[idstr];
    return write(fd, buf.data(), buf.size());
  }
  return file_be.send_and_cache(idstr, fd, cache_be[idstr]);
}
