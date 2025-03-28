# A playground for optimisation techniques

This codebase implements a simple system: a fileserver serving files according to a dead simple protocol, with a FIFO cache. However, it is also meant to be an exercise in using different features of modern C++ (up to C++20) to be a testbed for comparing different techniques for implementing this system:

Networking:
- `epoll`
- `epoll` + thread pool
- `io_uring`

Cache:
- `std::unordered_map`
- `khash`
- Google `sparsehash`

Filesystem:
- `open`
- `sqlite`

## Installation and Usage

## Results
