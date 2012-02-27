//====================================================================================
// mostly taken from ccache execute.c
//====================================================================================

#ifndef EXECUTE_H
#define EXECUTE_H

#include <stdio.h>
#include "chameleon.h"

int execute(char **argv,
            const char *path_stdout,
            const char *path_stderr);
char *find_executable(const char *name, const char *exclude_name);
void print_command(FILE *fp, char **argv);

#endif // EXECUTE_H
