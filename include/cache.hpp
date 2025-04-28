#pragma once

#include <vector>
#include <concepts>

struct DocData {
  size_t len;
  const void *ptr;
};

template <template <typename, typename> typename M, typename K, typename V>
concept HashMap = std::default_initializable<M<K, V>> && requires(M<K, V> m, K k) {
  { m[k] } -> std::convertible_to<V&>;
};

template <typename C>
concept DocCache = std::default_initializable<C> && requires(C cache, const std::vector<char>& buf) {
  { cache.get(buf) } -> std::convertible_to<DocData>;
};
