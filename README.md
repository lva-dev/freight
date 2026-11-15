# Freight
A cargo-like build system, but for C++.

## Build
Freight is built using CMake, so you can build it like any other CMake project:
```
cmake -S . -B build
```
Eventually, Freight will be built with Freight after support for libraries and incremental builds is added.

## Usage
### Overview
```
Usage: freight [OPTIONS] [COMMAND]

Options:
  -V, --version                  Print version info and exit
  -h, --help                     Print help

Commands:
    build, b    Compile the current package
    new         Create a new freight package
    init        Create a new freight package in an existing directory
    run, r      Run a binary of the local package
```

### Creating a new project
In a new directory:
```
freight new <PATH>
```
In the current directory or an existing directory:
```
freight init [PATH]
```
### Building a project
```
freight build [-r|--release]
```
### Running a project
```
freight run [-r|--release] [ARGS]...
```
