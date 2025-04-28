#pragma once

#include <chrono>
#include <concepts>
#include <string>
#include <vector>

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
    std::default_initializable<C> &&
    requires(C cache, const std::vector<char> &id, int fd, unsigned int ndocs) {
      { cache(ndocs) } -> std::same_as<C>;
      { cache.send(id, fd) } -> std::convertible_to<ll>;
    };

template <typename T>
concept FileBackend =
    requires(T backend, const fs::path &srv, const std::string &id,
             int fd, std::vector<char> &buf) {
      { backend(srv) } -> std::same_as<T>;
      { backend.cache(id, fd, buf) } -> std::convertible_to<ll>;
    };

struct FSBackend {
  fs::path dpath;
  FSBackend(const fs::path &srv);
  ll cache(const std::string &id, const int fd, std::vector<char> &buf);
};

#define CACHE_TEMPLATE                                                         \
  template <template <typename, typename> typename Map, typename Backend>      \
    requires HashMap<Map, std::string, std::vector<char>> &&                   \
             FileBackend<Backend>

// TODO implement generic cache logic, including LRU thread

// CACHE_TEMPLATE
// class BaseCache {
//   unsigned int ndocs;
//   Map<std::string, std::vector<char>> cache;
//
//   public:
//   FSCache(unsigned int _ndocs);
//   DocData get(const std::vector<char> &id);
// };
