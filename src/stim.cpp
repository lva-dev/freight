#include "pch.h"
#include "cmds.h"
#include "util.h"

static void invoke_command(std::string_view command, const ds::Strings& args) {
    if (command == "build") {
        exec_build(args);
    } else if (command == "init") {
        exec_init(args);
    } else if (command == "new") {
        exec_new(args);
    } else if (command == "run") {
        exec_run(args);
    } else {
        bail("unknown command '%s'", command);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        bail("not enough args");
        return 1;
    }

    const char *name = argv[1];
    ds::Strings cmd_args;
    for (int i = 2; i < argc; i++) {
        cmd_args.push_back(argv[i]);
    }
    
    invoke_command(name, cmd_args);
}
