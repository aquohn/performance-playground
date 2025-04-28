#pragma once

#include <filesystem>
#include <vector>
#include <format>
#include <syncstream>
#include <iostream>

namespace fs = std::filesystem;
using namespace std::literals;

typedef std::vector<unsigned char> hash_t;

hash_t sha256(std::vector<char> const &msg);

namespace pp {
  // format_string<Args...> allows argument type checks to be done at compile
  // time, avoiding runtime string exceptions
  template <typename... Args>
  void log_error(const std::format_string<Args...> fstr, Args&&... args) {
    static std::osyncstream syncerr(std::cerr);
    syncerr << std::format(fstr, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void fatal_error(const std::format_string<Args...> fstr, Args&&... args) {
    log_error(fstr, std::forward<Args>(args)...);
    exit(1);
  }
}
