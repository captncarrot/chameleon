#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "chameleon.h"
#include "util.h"
#include "execute.h"

//=============================================================================

char *logfile_name = 0;

/* current working directory taken from $PWD, or getcwd() if $PWD is bad */
char *current_working_dir = NULL;

/* the base cache directory */
char *cache_dir = NULL;

/* the debug logfile name, if set */
char *chameleon_logfile = NULL;

//=============================================================================

/** Analyse argument and return original or substitute.
  * @param arg    argument from command line
  * @return original value or according substitute
  */
char *analyse_arg(const char *arg, int argc)
{
  if (argc == 0)
  {
    char *name = find_executable(arg, MYNAME);
    if (name != 0)
      return name;
  }

  return x_strdup(arg);
}


/** Build the argument vector for the applicaton to run.
  * @param app_name name of application
  * @param arg_path name of argument file
  * @param oldargc  number of incoming arguments
  * @param oldargv  pointer to incoming arguments
  * @param newargc  [OUT] number of outgoing arguments
  * @param newargv  [OUT] pointer to outgoing arguments (caller frees)
  * @return \c true build succeeded
  */
bool build_new_argv(const char *app_name,
                    const char *arg_path,
                    int oldargc, char **oldargv,
                    int *newargc, char ***newargv)
{
  *newargc = 0;
  unsigned cntnext = oldargc + 5;
  char **pdest = *newargv = x_malloc(cntnext * sizeof(void*));
  *pdest = 0;

  // import argument file

  FILE *fp;
  fp = fopen(arg_path, "r");
  if (fp)
  {
    char buf[2000];
    do
    {
      char *p = fgets(buf, sizeof (buf), fp);
      if (p != 0)
      {
        int l = strlen(p);
        if (p[l - 1] == '\n')
          p[l - 1] = 0;

        *pdest++ = analyse_arg(p, *newargc);
        if ((*newargc)++ >= cntnext)
        {
          cntnext += 5;
          pdest = *newargv = x_realloc(pdest, cntnext * sizeof(void*));
        }
      }

      if (ferror(fp))
      {
        cc_log("Could not read argument file %s", arg_path);
        return false;
      }

    } while (!feof(fp));
  }

  else
  {
    cc_log("Could not open argument file %s", arg_path);
    return false;
  }
  fclose(fp);

  // import command line

  int i = 1;
  while (i < oldargc)
  {
    *pdest++ = analyse_arg(oldargv[i++], *newargc);
    if ((*newargc)++ >= cntnext)
    {
      cntnext += 5;
      pdest = *newargv = x_realloc(pdest, cntnext * sizeof(void*));
    }
  }

  return true;
}

//=============================================================================

/** Run the original application
  * @param argc   number of arguments
  * @param argv   pointers to values
  * @return return value from application
  */
int run_app(int argc, char **argv)
{
  return execute(argv, NULL, NULL);
}

//=============================================================================

int main(int argc, char **argv)
{
  current_working_dir = get_cwd();
  if (!current_working_dir)
  {
    cc_log("Could not determine current working directory");
    return 1;
  }

  char *arg_path = x_malloc(strlen(argv[0]) + 4 + 1); // "/" + ".arg"

#if defined __CYGWIN__
  const char *sep = "/";
#elif defined __WIN32__
  const char *sep = "\\";
#else
  const char *sep = "/";
#endif

  char *app_name = basename(argv[0]);
  char *app_path = dirname(argv[0]);
  if (strlen(app_path) == 0)
  {
    char *p = find_executable(app_name, MYNAME);
    if (app_path == NULL)
      return 1;

    free(app_path);
    app_path = dirname(p);
  }

#ifdef __WIN32__
  const unsigned l = strlen(app_name);
  if (strcasecmp(app_name + l -4, ".exe") == 0)
  app_name[l - 4] = 0;
#endif

  strcpy(arg_path, app_path);
  strcat(arg_path, sep);
  strcat(arg_path, app_name);
  strcat(arg_path, ".arg");

  char **newargv = 0;
  int newargc = 0;

  int ret = 1;
  if(build_new_argv(app_path, arg_path, argc, argv, &newargc, &newargv))
    ret = run_app(newargc, newargv);

  free(app_path);
  free(arg_path);

  while (newargv && *newargv)
    free (*newargv++);

  return ret;
}
