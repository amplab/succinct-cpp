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
* Thrift for its `thrift`, `adaptive`, `bench` and `test` modules.

# Building

To build Succinct, run:

```
./build.sh
```

To clean-up build files, run:

```
./cleanup.sh
```

# Constructing datasets

TODO: Add description of how `construct` tool can be used.

# Starting Succinct as a Service

TODO: Add description of how `sbin/` scripts can be used.
