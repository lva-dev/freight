# Freight
A cargo-like build system, but for C++.
## Build
Freight is built using CMake, so you can build it like any other CMake project:
```
cmake -S . -B build
```
or, you can run the [build script](scripts/build.sh) in the `scripts` directory (must have CMake installed):
```
chmod +x scripts/build.sh
scripts/build.sh
```
## Usage
### Overview
```
Usage: freight [options] [COMMAND]

Options:
  -V, --version   Print version info and exit
  -h, --help      Print help

Commands:
  build, b   Compile the current project
  new        Create a new freight project
  init       Create a new freight project in an existing directory
  run, r     Run a binary of the local project
```

### Creating a new project
In a new directory:
```
freight new <PATH>
```
In the current directory:
```
freight init
```
### Building a project
```
freight build
```
### Running a project
```
freight run
```
## Notes
`freight new` is the only command implemented at the moment. `init`, `build`, and `run` should be completed within a few weeks (from Dec. 15, 2025, when this was written).