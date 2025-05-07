# A playground for optimisation techniques

This codebase implements a simple system: a fileserver serving files according to a dead simple protocol, with an LRU cache. However, it is also meant to be an exercise in using different features of modern C++ (up to C++20), and a testbed for comparing different techniques for implementing this system. The backends used to implement the various components of the system are:

Networking:
- `epoll`
- `epoll` + thread pool (WIP)
- `io_uring` (WIP)

Cache:
- `std::unordered_map`
- None (WIP)
- `khash` (WIP)
- Google `sparsehash` (WIP)

Cache Eviction:
- None
- Mutex
- Lockfree queue and hashmap (WIP)

Filesystem:
- `sendfile`
- `sqlite` (["SQLite does not compete with client/server databases. SQLite competes with fopen()."](https://www.sqlite.org/whentouse.html#:~:text=SQLite%20competes%20with%20fopen())) (WIP)

## Compilation and Usage

In addition to the included submodules, building this project requires the following packages:

```
openssl sparsehash sqlite3
```

Depending on your distribution, you may need to install the `-devel` versions of these packages.

The usages for the server and client are as follows:

```
Usage: ./build/server name> -l <loop> -r <request map> -c <cache eviction> -m <cache map> -f <file backend> [ -n cache capacity ] <port> <content directory>
```

```
Usage: ./build/client [-a avg sleep ms] [-d sleep stddev ms] [-r num requestors] [-n num reps per requestor] <server addr> <content directory>
``

## Design

The system and its components are chosen to minimise latency where possible (where deemed infeasible, comments are added explaining design choices). Static polymorphism is used throughout and virtual methods are avoided.

## Results

TODO
