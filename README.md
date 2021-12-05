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

Clone 该仓库。

```shell
$ git clone https://github.com/kejiyuan306/protocon-cpp
```

切换工作路径到项目目录。

```shell
$ cd protocon-cpp
```

构建项目。

```shell
$ xmake -ay
```

运行测试。

```shell
$ xmake run Tests
```

运行 benchmarks。

```shell
$ xmake run Benches
```

可以使用如下命令安装本类库。

```shell
$ xmake install
```
