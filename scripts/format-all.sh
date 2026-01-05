#!/usr/bin/bash

if [[ -d src ]]; then
  find src \
    '(' ! -wholename '*/vendor/*' -a '(' -name '*.cpp' -o -name '*h' -o -name '*.hpp' ')' ')' \
    -exec clang-format -i {} \;
fi