#pragma once

#include <concepts>
#include <string>
#include <vector>
#include <unordered_map>

#include <unistd.h>

#include "utils.hpp"

// similar constraints can be implemented using pure virtual base classes, final
// inheritance, and derived_from, with similar performance

template <template <typename, typename> typename M, typename K, typename V>
concept HashMap =
    std::default_initializable<M<K, V>> && requires(M<K, V> m, K k) {
      { m.contains(k) } -> std::convertible_to<bool>;
      { m[k] } -> std::convertible_to<V &>;
    };

// TODO make cache look at total size rather than number of docs
template <typename C>
concept DocCache =
    std::constructible_from<C, const fs::path &, unsigned int> &&
    requires(C cache, const std::string &idstr, const int fd) {
      { cache.send(idstr, fd) } -> std::convertible_to<ll>;
    };

template <typename T>
concept FileBackend =
    std::constructible_from<T, const fs::path &> &&
    requires(T backend, const fs::path &srv, const std::string &idstr,
             const int fd, std::vector<char> &buf) {
      { backend.send_and_cache(idstr, fd, buf) } -> std::convertible_to<ll>;
    };

#define CACHE_TEMPLATE                                                         \
  template <template <typename, typename> typename Map, typename Backend>      \
    requires HashMap<Map, std::string, std::vector<char>> &&                   \
             FileBackend<Backend>

// TODO implement LRU thread with cv to signal need for eviction

CACHE_TEMPLATE
class BaseCache {
  unsigned int ndocs;
  Map<std::string, std::vector<char>> cache;
  Backend backend;

  public:
  BaseCache(const fs::path &srv, unsigned int _ndocs) : ndocs(_ndocs), backend(srv) {}
  ll send(const std::string &idstr, const int fd);
};

CACHE_TEMPLATE
ll BaseCache<Map, Backend>::send(const std::string &idstr, const int fd) {
  if (cache.contains(idstr)) {
    std::vector<char>& buf = cache[idstr];
    return write(fd, buf.data(), buf.size());
  }
  // TODO proper handling
  return backend.send_and_cache(idstr, fd, cache[idstr]);
}

template<typename K, typename V> struct UMap {
  std::unordered_map<K, V> umap;
  bool contains(const K& k) {
    return umap.find(k) != umap.end();
  }
  V& operator[](const K& k) {
    return umap[k];
  }
  V& operator[](K&& k) {
    return umap[k];
  }
};

struct FSBackend {
  fs::path dpath;
  FSBackend(const fs::path &srv);
  ll send_and_cache(const std::string &idstr, const int fd, std::vector<char> &buf);
};

