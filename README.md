# go_data_gen

## Purpose

This repository provides a C++ library with Python bindings for generating data for the game of Go. The library includes functionality for loading SGF files, manipulating Go boards, and generating input data for training NNs.

## Prerequisites

### PyTorch
Build libtorch from source.
Install it to a certain directory of your choice, then point `find_package(Torch)` in the root CMakeLists.txt to it with the PATH parameter.

### Other dependencies
Other dependencies should be downloaded and built during the build process automatically.

## Building the C++ Library

CMake-based build:

```sh
mkdir build
cd build
cmake ..
cmake --build .
```

## Building and Installing the Python Library

Installation with `pip`:

```sh
pip install .
```

## Usage

See `examples/play_sgf.cpp` and `examples/play_sgf.py`.
After executing the build (and installation) step(s) above, you can run the example:

```sh
./build/examples/play_sgf <path_to_sgf_file>
```

Python version:

```sh
python examples/play_sgf.py <path_to_sgf_file>
```
