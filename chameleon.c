#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/time.h>

#include "chameleon.h"
#include "util.h"
#include "execute.h"

//====================================================================================

#define HAVE_GETTIMEOFDAY
#define HAVE_STRNDUP

//====================================================================================

FILE *logfile;
char *logfile_name = 0;
char *current_working_dir = NULL;

//====================================================================================

//====================================================================================

void failed()
{
  exit(1);
}

//====================================================================================

int main(int argc, char **argv)
{
  current_working_dir = get_cwd();
  if (!current_working_dir)
  {
    cc_log("Could not determine current working directory");
    failed();
  }

  const char *master = x_strdup(argv[0]);

#if defined __CYGWIN__
  const char *sep = "/";
#elif defined __WIN32__
  const char *sep = "\\";
#else
  const char *sep = "/";
#endif

  const char *name = master;
  char *pc;
  while (NULL != (pc = strstr(name, sep) ))
    name = pc + 1;

#ifdef __WIN32__
  const unsigned l = strlen(name);
  if (strcasecmp(name + l -4, ".exe") == 0)
    name[l - 4] = 0;
#endif

  return 0;
}