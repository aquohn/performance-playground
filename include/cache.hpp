#pragma once

#include <cassert>
#include <concepts>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <unistd.h>

#include "utils.hpp"

// similar constraints can be implemented using pure virtual base classes, final
// inheritance, and derived_from, with similar performance

// TODO indicate lock-freedom with type traits, for testing compatibility with
// lockfree caches
template <template <typename, typename> typename M, typename K, typename V>
concept HashMap =
    std::default_initializable<M<K, V>> && requires(M<K, V> m, const K &k) {
      { m.contains(k) } -> std::convertible_to<bool>;
      { m[k] } -> std::convertible_to<V &>;
      { m.size() } -> std::convertible_to<ull>;
      m.erase(k);
    };

// TODO make cache look at total size rather than number of docs
template <typename C>
concept DocCache = std::constructible_from<C, const fs::path &, unsigned int> &&
                   requires(C cache, const std::string &idstr, const int fd) {
                     { cache.send(idstr, fd) } -> std::convertible_to<ll>;
                   };

template <typename T>
concept FileBackend =
    std::constructible_from<T, const fs::path &> &&
    requires(T file_be, const fs::path &srv, const std::string &idstr,
             const int fd, std::vector<char> &buf) {
      { file_be.send_and_cache(idstr, fd, buf) } -> std::convertible_to<ll>;
    };

#define CACHE_TEMPLATE                                                         \
  template <template <typename, typename> typename Map, typename Backend>      \
    requires HashMap<Map, std::string, std::vector<char>> &&                   \
             FileBackend<Backend>

CACHE_TEMPLATE
class BaseCache {
protected:
  ull ndocs;
  Map<std::string, std::vector<char>> cache_be;
  Backend file_be;

public:
  BaseCache(const fs::path &srv, ull _ndocs) : ndocs(_ndocs), file_be(srv) {}
  ll send(const std::string &idstr, const int fd);
};

CACHE_TEMPLATE &&HashMap<Map, std::string, ull> struct MutexCache
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

CACHE_TEMPLATE
ll BaseCache<Map, Backend>::send(const std::string &idstr, const int fd) {
  if (cache_be.contains(idstr)) {
    std::vector<char> &buf = cache_be[idstr];
    return write(fd, buf.data(), buf.size());
  }
  return file_be.send_and_cache(idstr, fd, cache_be[idstr]);
}

template <typename K, typename V>
struct UMap : public std::unordered_map<K, V> {
  bool contains(const K &k) { return this->find(k) != this->end(); }
};

struct FSBackend {
  fs::path dpath;
  FSBackend(const fs::path &srv);
  ll send_and_cache(const std::string &idstr, const int fd,
                    std::vector<char> &buf);
};
