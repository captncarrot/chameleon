//====================================================================================
// mostly taken from ccache ccache.h
//====================================================================================

#ifndef CHAMELEON_H
#define CHAMELEON_H

//====================================================================================

#ifdef __GNUC__
#define ATTR_FORMAT(x, y, z) __attribute__((format (x, y, z)))
#else
#define ATTR_FORMAT(x, y, z)
#endif

#ifndef MYNAME
#define MYNAME "chameleon"
#endif

typedef int bool;
#define true 1
#define false 0

#if HAVE_COMPAR_FN_T
#define COMPAR_FN_T __compar_fn_t
#else
typedef int (*COMPAR_FN_T)(const void *, const void *);
#endif

/* work with silly DOS binary open */
#ifndef O_BINARY
#define O_BINARY 0
#endif

/* mkstemp() on some versions of cygwin don't handle binary files, so
   override */
#ifdef __CYGWIN__
#undef HAVE_MKSTEMP
#endif

#ifdef _WIN32
int win32execute(char *path, char **argv, int doreturn,
                 const char *path_stdout, const char *path_stderr);
#    ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0501
#    endif
#    include <windows.h>
#    define mkdir(a,b) mkdir(a)
#    define link(src,dst) (CreateHardLink(dst,src,NULL) ? 0 : -1)
#    define lstat(a,b) stat(a,b)
#    define execv(a,b) win32execute(a,b,0,NULL,NULL)
#    define execute(a,b,c) win32execute(*(a),a,1,b,c)
#    define PATH_DELIM ";"
#    define F_RDLCK 0
#    define F_WRLCK 0
#else
#    define PATH_DELIM ":"
#endif

#define str_eq(s1, s2) (strcmp((s1), (s2)) == 0)
#define str_startswith(s, p) (strncmp((s), (p), strlen((p))) == 0)

#endif // CHAMELEON_H
