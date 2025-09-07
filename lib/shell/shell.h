#ifndef _INC_SHELL_H
#define _INC_SHELL_H

struct ShellCommand {
    char *name;
    int (*execute)(int argc, char *argv[]);
};

void Shell_Spin(const struct ShellCommand *const *commands);

#endif // _INC_SHELL_H