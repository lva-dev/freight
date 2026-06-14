[[ ! -v STIM_EXECUTABLE ]] && STIM_EXECUTABLE="$(realpath $(dirname "$(dirname "$(dirname ${BASH_SOURCE[0]})")")/build/freight)"
[[ ! -v STIM_CURRENT_TEST_DIR ]] && STIM_CURRENT_TEST_DIR="$(mktemp -d)"
[[ ! -v STIM_CURRENT_PROJECT_NAME ]] && STIM_CURRENT_PROJECT_NAME=abc
[[ ! -v STIM_CURRENT_PROJECT_DIR ]] && STIM_CURRENT_PROJECT_DIR="$STIM_CURRENT_TEST_DIR/$STIM_CURRENT_PROJECT_NAME"

function build-debug() {
  local out="$(cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug 2>&1)"
  local err=$?

  if ((err != 0)) then
    echo "$out" >&2
  else
    cmake --build build -j 16
  fi
}

function test-freight-new() {
  if [[ ! -v STIM_CURRENT_TEST_DIR ]]; then
    echo -e "\e[31merror\e[m: 'STIM_CURRENT_TEST_DIR' has not been set" >&2
    return 1
  fi

  if [[ ! -d "$STIM_CURRENT_TEST_DIR" ]]; then
    echo -e "\e[31merror\e[m: '$STIM_CURRENT_TEST_DIR' does not exist or is not a directory" >&2
    return 1
  fi
  
  pushd "$STIM_CURRENT_TEST_DIR" >/dev/null && "$STIM_EXECUTABLE" new "$(basename "$STIM_CURRENT_PROJECT_DIR")" "$@"
  popd >/dev/null
}

function test-freight-new-gdb() {
  if [[ ! -v STIM_CURRENT_TEST_DIR ]]; then
    echo -e "\e[31merror\e[m: 'STIM_CURRENT_TEST_DIR' has not been set" >&2
    return 1
  fi

  if [[ ! -d "$STIM_CURRENT_TEST_DIR" ]]; then
    echo -e "\e[31merror\e[m: '$STIM_CURRENT_TEST_DIR' does not exist or is not a directory" >&2
    return 1
  fi
  
  pushd "$STIM_CURRENT_TEST_DIR" >/dev/null && gdb --args "$STIM_EXECUTABLE" new "$(basename "$STIM_CURRENT_PROJECT_DIR")" "$@"
  popd >/dev/null
}

function test-freight() {
  if [[ ! -v STIM_CURRENT_PROJECT_DIR ]]; then
    echo -e "\e[31merror\e[m: 'STIM_CURRENT_PROJECT_DIR' has not been set" >&2
    return 1
  fi

  if [[ ! -d "$STIM_CURRENT_PROJECT_DIR" ]]; then
    echo -e "\e[31merror\e[m: '$STIM_CURRENT_PROJECT_DIR' does not exist or is not a directory" >&2
    return 1
  fi
  
  pushd "$STIM_CURRENT_PROJECT_DIR" >/dev/null && "$STIM_EXECUTABLE" "$@"
  popd >/dev/null
}

function test-freight-gdb() {
  if [[ ! -v STIM_CURRENT_PROJECT_DIR ]]; then
    echo -e "\e[31merror\e[m: 'STIM_CURRENT_PROJECT_DIR' has not been set" >&2
    return 1
  fi

  if [[ ! -d "$STIM_CURRENT_PROJECT_DIR" ]]; then
    echo -e "\e[31merror\e[m: '$STIM_CURRENT_PROJECT_DIR' does not exist or is not a directory" >&2
    return 1
  fi
  
  pushd "$STIM_CURRENT_PROJECT_DIR" >/dev/null && gdb --args "$STIM_EXECUTABLE" "$@"
  popd >/dev/null
}

function test-freight-valgrind() {
  if [[ ! -v STIM_CURRENT_PROJECT_DIR ]]; then
    echo -e "\e[31merror\e[m: 'STIM_CURRENT_PROJECT_DIR' has not been set" >&2
    return 1
  fi

  if [[ ! -d "$STIM_CURRENT_PROJECT_DIR" ]]; then
    echo -e "\e[31merror\e[m: '$STIM_CURRENT_PROJECT_DIR' does not exist or is not a directory" >&2
    return 1
  fi
  
  pushd "$STIM_CURRENT_PROJECT_DIR" >/dev/null && valgrind --leak-check=full "$STIM_EXECUTABLE" "$@"
  popd >/dev/null
}