#!/usr/bin/env bash

cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "$@"