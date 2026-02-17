# libcaparoc

CAPAROC library built on top of `modbus_cpp`.

## Prerequisites

- CMake 3.25+
- C++23 compiler (examples use `g++-14`)
- `pkg-config`
- `libmodbus` development package (`libmodbus-dev` on Debian/Ubuntu)

Example dependency install (Debian/Ubuntu):

```bash
sudo apt-get update
sudo apt-get install -y cmake pkg-config libmodbus-dev
```

## Build

`libcaparoc` fetches `libmodbus-cpp` via `FetchContent`.

Use a fresh build directory, especially when switching compilers:

```bash
rm -rf build
```

Build:

```bash
cd libcaparoc
rm -rf build
CXX=g++-14 cmake -D CMAKE_BUILD_TYPE=Debug -B build -S .
cmake --build build -j4
```
