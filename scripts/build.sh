#!/usr/bin/env bash
#
# usage:
#  build.sh [debug|release|minsizerel|relwithdebinfo]

# configuration:
#  (set OUTPUT_DIRECTORY to nothing for its default value)

TARGET=freight
OUTPUT_DIRECTORY=

##################################################

function generate-build-files {
	cmake -S . -B build \
		-DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
		"${OUTPUT_DIRECTORY:+-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=${OUTPUT_DIRECTORY}}" \
		"${OUTPUT_DIRECTORY:+-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${OUTPUT_DIRECTORY}}" \
		"${OUTPUT_DIRECTORY:+-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${OUTPUT_DIRECTORY}}" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
}

function build {
	cmake --build build --config "${CMAKE_BUILD_TYPE}"
}

function print {
	echo -e "  \033[3m$*\033[23m"
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
		--help | -h)
			echo "usage:"
			echo "  $(basename "${0}") [debug|release|minsizerel|relwithdebinfo]"
			exit 0
			;;
		debug | deb)
			CMAKE_BUILD_TYPE=Debug
			;;
		release | rel)
			CMAKE_BUILD_TYPE=Release
			;;
		minsizerel | msr | size)
			CMAKE_BUILD_TYPE=MineSizeRel
			;;
		relwithdebinfo | debugrel)
			CMAKE_BUILD_TYPE=RelWithDebInfo
			;;
		*)
			error "unknown build type '$1'"
			return 1
			;;
		esac
	fi

	CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Debug}"

	print '\033[35mgenerating\033[39m build files (with cmake)...'
	if generate-build-files; then
		print '\033[32msuccesfully\033[39m generated build files'
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
