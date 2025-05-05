#include <chrono>

#include <sys/ioctl.h>

#include "loop.hpp"

void SocketGC::gc() {
  std::unique_lock lk(mtx);
  while (true) {
    // possible alternative: release and reacquire mutex after each fd is closed
    // may decrease latency but not sure if will lead to fd starvation at scale
    while (!q.empty()) {
      int fd = q.front();
      int rem, n;
      if (ioctl(fd, TIOCOUTQ, &rem) < 0 || ioctl(fd, FIONREAD, &n) < 0) {
        pp::log_error("Error when checking socket data: {}\n", strerror(errno));
        close(fd);
        q.pop();
        continue;
      }

      if (rem > 0) {
        pp::debug("gc: {} bytes left to send, not closing yet\n", rem);
        break;
      }
      if (n == 0) {
        pp::debug("gc: socket closed by remote peer", rem);
        close(fd);
        break;
      }
      pp::debug("gc: closing socket\n");
      close(fd);
      q.pop();
      continue;
    }

    if (q.empty()) {
      cv.wait(lk);
    } else {
      lk.unlock();
      std::this_thread::sleep_for(
          std::chrono::duration<double, std::milli>{GC_INTERVAL_MS});
      lk.lock();
    }
  }
}
