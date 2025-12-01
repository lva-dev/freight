#include "pch.h"
#include <cstdlib>

#include "cli.h"

int main(int argc, char **argv) {
	freight::Cli cli;
	cli.start(argc, argv);
	return EXIT_SUCCESS;
}