#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "klib/cpp/khashl.hpp"

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

template <typename T, typename IDT = std::string,
          typename BufT = std::vector<char>>
concept FileBackend = std::constructible_from<T, const fs::path &> &&
                      requires(T file_be, const fs::path &srv, const IDT &id,
                               const int fd, BufT &buf) {
                        {
                          file_be.send_and_cache(id, fd, buf)
                        } -> std::convertible_to<ll>;
                      };

// Maps

template <typename K, typename V>
struct UMap : public std::unordered_map<K, V> {
  bool contains(const K &k) { return this->find(k) != this->end(); }
};

struct kh_hash_string {
  uint32_t operator()(const std::string &s);
};
template <typename K, typename V, typename Hash>
struct BaseKHashMap : public klib::KHashMap<K, V, Hash> {
  bool contains(const K &k) {
    return this->get(k) == this->end();
  }

  int erase(const K &k) {
    return this->del(this->get(k));
  }
};
template <typename K, typename V>
struct KHashMap : public BaseKHashMap<K, V, std::hash<K>> {};
template <typename V>
struct KHashMap<std::string, V> : public BaseKHashMap<std::string, V, kh_hash_string> {};

// File backends

struct FSBackend {
  fs::path dpath;
  FSBackend(const fs::path &srv);
  ll send_and_cache(const std::string &idstr, const int fd,
                    std::vector<char> &buf);
};
