#pragma once

#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <print>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <dirent.h>
#include <libgen.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>