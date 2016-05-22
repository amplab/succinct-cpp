# Succinct

C++ Implementation of Succinct (http://succinct.cs.berkeley.edu).

Succinct is a data-store than enables queries directly on a compressed
representation of data. With basic primitives of random acces, search
and fast counts of arbitrary substrings on unstructured data (flat files),
Succinct can provide a rich set of queries on semi-structured or structured
data without requiring any secondary indexes.

# Requirements

* C++11 support
* CMake build system
* Thrift for its `sharded`, `sharded-kv`, `bench` and `test` modules.

# Building

To build Succinct, run:

```
./build.sh
```

To clean-up build files, run:

```
./cleanup.sh
```

# Examples

To see how Succinct's core data structures can be used, find examples in [examples/src](examples/src).

The example in [compress.cc](examples/src/compress.cc) shows how Succinct can be used to compress input files (for both flat-file and key-value interfaces), while example programs in [query_file.cc](examples/src/query_file.cc) and [query_kv.cc](examples/src/query_kv.cc) show how the compressed files can be queried.

# Starting Succinct as a Service

TODO: Add description of how `sbin/` scripts can be used.
