# Protocon

[![C++ CI with CMake](https://github.com/kejiyuan306/protocon-cpp/actions/workflows/cmake.yml/badge.svg)](https://github.com/kejiyuan306/protocon-cpp/actions/workflows/cmake.yml)

本项目是 Protocon 协议的 C++ 实现，对于发送命令请求与接受命令响应进行了简单的封装。

## Features

- Test with GoogleTest
- Benchmark with Google benchmark
- Code formatting with ClangFormat
- Simple installing and exporting

## Requirements

- CMake (version 3.14 or later)
- C++ Compiler (such as GCC, Clang or MSVC)

## Quick start

Clone the repository

```shell
$ git clone https://github.com/kejiyuan306/protocon-cpp
```

Navigate to project directory.

```shell
$ cd protocon-cpp
```

Create a directory for build, and then navigate to it.

```shell
$ mkdir build
$ cd build
```

Generate the project buildsystem.

```shell
$ cmake .. -DCMAKE_BUILD_TYPE=Release
```

Build the project.

```shell
$ cmake --build . --config Release
```

Run tests.

```shell
$ ctest
```

The project can be installed as follows.

```shell
$ cmake --install . --config Release
```

## Import the library

There're two ways to import this library.

### Subdirectory

Copy the whole project to a directory used for placing external dependencies, such as `extern`.

Put a `add_subdirectory(protocon-cpp)` in `CMakeLists.txt`.

The library can be linked like `target_link_libraries(<Other Target> PRIVATE Protocon::Protocon)`.

### Find package

Install the library as described above.

Put a `find_package(Protocon 2.2.1 REQUIRED)` in `CMakeLists.txt`.

Link the library like `target_link_libraries(<Other Target> PRIVATE Protocon::Protocon)`.
