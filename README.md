# A playground for optimisation techniques

This codebase implements a simple system: a fileserver serving files according to a dead simple protocol, with an LRU cache. However, it is also meant to be an exercise in using different features of modern C++ (up to C++20), and a testbed for comparing different techniques for implementing this system. The backends used to implement the various components of the system are:

Networking:
- `epoll`
- `epoll` + thread pool (WIP)
- `io_uring` (WIP)

Cache:
- None
- `std::unordered_map`
- `khash` (buggy; potential memory leak due to upstream issues)
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
```

Currently, the supported format for the content directory is a set of subdirectories with a distinct 8-character identifier, with characters from the set `[A-Z0-9]`, and with a single `.pdf` file under each subdirectory. This is the format of my Zotero storage directory. A plugin system to support other content directory formats is being developed, although this would unavoidably require the use of virtual methods.

## Design

The server and its components are designed to minimise latency where possible (where deemed infeasible, comments are added explaining design choices). Static polymorphism is used throughout and virtual methods are avoided. Template metaprogramming is used to instantiate all possible combinations of components at compile time to allow dynamic selection of backends without sacrificing performance.

The components for the network loop, cache eviction policy, file access backend, and mappings used by the cache all have their API constrained by concepts, enforcing the interface between them without the costs of dynamic polymorphism.

The client consists of a number of requestor threads, which will make a fixed, total number of requests to the server and sleep for a random, configurable time. The mean, standard deviation, and worst round-trip latency are reported when the client exits.

## Results

Results measured with 10 requestor threads requesting 30 documents over loopback.

|Loop   |Requests|Cache Eviction|Cache Map|File Backend|Avg (ms)        |Worst (ms)        |
|---    |---     |---           |---      |---         |---             |---               |
|`epoll`|`umap`  |`mutex`       |`none`   |`filesystem`|0.51 +/- 0.52   |26.26             |
|`epoll`|`umap`  |`mutex`       |`umap`   |`filesystem`|1.10 +/- 1.11   |46.33             |
|`epoll`|`umap`  |`none`        |`none`   |`filesystem`|0.30 +/- 0.30   |22.25             |
|`epoll`|`umap`  |`none`        |`umap`   |`filesystem`|0.58 +/- 0.58   |23.37             |

Disabling caching actually improves average and worst-case latency. However, as the performance is similar to the cases with no cache eviction, this is likely due to not needing to run the GC thread to evict the cache entries. Caching itself does not seem to give much benefit, likely since the access patterns have not (yet!) been tuned to follow an LRU distribution which would work better with the cache.
