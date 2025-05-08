#include <cstring>

#include <fcntl.h>
#include <sys/sendfile.h>

#include "backend.hpp"
#include "cache.hpp"

#include "klib/khashl.h"

FSBackend::FSBackend(const fs::path &srv) : dpath(srv) {}

ll FSBackend::send_and_cache(const std::string &idstr, const int fd,
                             std::vector<char> &buf) {
  fs::path fpath;
  if (!find_pdf(dpath / idstr, fpath)) {
    return -1;
  }

  size_t fsz = fs::file_size(fpath);
  int filefd = open(fpath.c_str(), O_RDONLY);
  if (filefd < 0) {
    pp::log_error("Error opening {}: {}\n", fpath.string(), strerror(errno));
    return -1;
  }

  ll sent = sendfile(fd, filefd, NULL, fsz);
  if (sent < 0) {
    pp::log_error("Error sending {}: {}\n", fpath.string(), strerror(errno));
    close(filefd);
    return -1;
  }

  if (lseek(filefd, 0, SEEK_SET) < 0) {
    pp::log_error("Error seeking {}: {}\n", fpath.string(), strerror(errno));
    close(filefd);
    return -1;
  }
  buf.resize(fsz);
  if (read(filefd, buf.data(), fsz) < 0) {
    pp::log_error("Error reading {}: {}\n", fpath.string(), strerror(errno));
    close(filefd);
    return -1;
  }

  close(filefd);
  return sent;
}

uint32_t kh_hash_string::operator()(const std::string &s) { return kh_hash_str(s.c_str()); }
