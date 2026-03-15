#include "pch.h"
#include "cmds.h"
#include "util.h"

void invoke_command(const char *command, Strs args) {
    if (streq(command, "new")) {
        exec_new(args);
    } else if (streq(command, "run")) {
        exec_run(args);
    } else {
        bail("unknown command '%s'", command);
    }
}

int main(int argc, char **argv) {
    Strs args = strs_from_array(argv, argc);
    if (argc < 2) {
        bail("not enough args");
        return 1;
    }

    const char *name = argv[1];
    Strs cmd_args = strs_subrange(args, 2, -1);
    invoke_command(name, cmd_args);
    strs_free(cmd_args);
}
