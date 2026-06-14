#pragma once

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

#include <algorithm>
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
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifdef __linux__
	#include <dirent.h>
	#include <libgen.h>
	#include <sys/mman.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <sys/wait.h>
	#include <unistd.h>
#else
	#error "Unsupported platform"
#endif

#define TOML_EXCEPTIONS 0
#include "Toml.h"
