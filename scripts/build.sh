#!/usr/bin/env bash
#
# usage:
#  build.sh [debug|release|minsizerel|relwithdebinfo]

### Configuration

TARGET=project

### End of configuration

function generate-build-files {
  cmake -S . -B build \
    -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
}

function build {
  cmake --build build
}

function print {
  echo -e "\033[3m$*\033[23m"
}

function error {
  print "\033[31merror\033[39m: $*"
}

declare CMAKE_BUILD_TYPE

function main {
  shopt -s nocasematch
  
  if [[ $# -gt 1 ]]; then
    error "too many arguments"
    return 1
  fi

  if [[ $# -eq 1 ]]; then
    case $1 in
      --help|-h)
        echo "usage:"
        echo "  $(basename "${0}") [debug|release|minsizerel|relwithdebinfo]"
        exit 0;;
      debug|deb)
        CMAKE_BUILD_TYPE=Debug;;
      release|rel)
        CMAKE_BUILD_TYPE=Release;;
      minsizerel|msr|size)
        CMAKE_BUILD_TYPE=MineSizeRel;;
      relwithdebinfo|debugrel)
        CMAKE_BUILD_TYPE=RelWithDebInfo;;
      *)
        error "unknown build type '$1'"
        return 1
    esac
  fi

  CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Debug}"

  print '\033[35mgenerating\033[39m cmake build files...'
  if generate-build-files; then 
    print '\033[32msuccesfully\033[39m generated cmake build files'
  else
    error 'failed to generate cmake build files' >&2
    return 1
  fi

  print "\033[35mbuilding\033[39m ${TARGET}..."
  if build; then
    print "\033[32msuccessfully\033[39m built ${TARGET}"
  else
    error 'build failed' >&2
    return 1
  fi

  return 0
}

main "$@"