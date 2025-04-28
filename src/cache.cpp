#include <sys/sendfile.h>
#include <fcntl.h>

#include "cache.hpp"

FSBackend::FSBackend(const fs::path &srv) : dpath(srv) {}

ll FSBackend::send_and_cache(const std::string &idstr, const int fd, std::vector<char> &buf) {
  fs::path fpath;
  if (!find_pdf(dpath / idstr, fpath)) {
    return -1;
  }

  size_t fsz = fs::file_size(fpath);
  int filefd = open(fpath.c_str(), O_RDONLY);
  if (filefd < 0) {
    return -1;
  }

  ll sent = sendfile(fd, filefd, NULL, fsz);
  if (sent < 0) {
    close(filefd);
    return -1;
  }

  if (lseek(filefd, 0, SEEK_SET) < 0) {
    close(filefd);
    return -1;
  }
  buf.resize(fsz);
  if (read(filefd, buf.data(), fsz) < 0) {
    close(filefd);
    return -1;
  }

  close(filefd);
  return sent;
}

