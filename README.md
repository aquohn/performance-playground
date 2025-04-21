# A playground for optimisation techniques

This codebase implements a simple system: a fileserver serving files according to a dead simple protocol, with a FIFO cache. However, it is also meant to be an exercise in using different features of modern C++ (up to C++20), and a testbed for comparing different techniques for implementing this system. The backends used to implement the various components of the system are:

Networking:
- `epoll`
- `epoll` + thread pool (WIP)
- `io_uring` (WIP)

Cache:
- None
- `std::unordered_map` (WIP)
- `khash` (WIP)
- Google `sparsehash` (WIP)

Filesystem:
- `open`
- `sqlite` (["SQLite does not compete with client/server databases. SQLite competes with fopen()."](https://www.sqlite.org/whentouse.html#:~:text=SQLite%20competes%20with%20fopen()))

## Compilation and Usage

## Results
