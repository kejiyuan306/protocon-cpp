# Protocon

[![C++ CI with xmake](https://github.com/kejiyuan306/protocon-cpp/actions/workflows/xmake.yml/badge.svg)](https://github.com/kejiyuan306/protocon-cpp/actions/workflows/xmake.yml)

本项目是 Protocon 协议的 C++ 实现，对于发送命令请求与接受命令响应进行了简单的封装。

## Features

- Test with GoogleTest
- Benchmark with Google benchmark
- Code formatting with ClangFormat
- Simple installing and exporting

## Requirements

- xmake (version 2.5.9 or later)
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

Build the project.

```shell
$ xmake -ay
```

Run tests.

```shell
$ xmake run Tests
```

Run benchmarks

```shell
$ xmake run Benches
```

The project can be installed as follows.

```shell
$ xmake install
```
