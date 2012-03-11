//====================================================================================
// mostly taken from ccache util.c
//====================================================================================

/*
 * Copyright (C) 2002 Andrew Tridgell
 * Copyright (C) 2009-2012 Joel Rosdahl
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include "util.h"
#include "execute.h"
#include "chameleon.h"

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef _WIN32
#include <sys/dirent.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <sys/locking.h>
#endif

static FILE *logfile;

static bool init_log(void)
{
  extern char *chameleon_logfile;

  if (logfile)
  {
    return true;
  }
  if (!chameleon_logfile)
  {
    return false;
  }
  logfile = fopen(chameleon_logfile, "a");
  if (logfile)
  {
    return true;
  }
  else
  {
    return false;
  }
}

static void log_prefix(void)
{
#ifdef HAVE_GETTIMEOFDAY
  char timestamp[100];
  struct timeval tv;
  struct tm *tm;

  gettimeofday(&tv, NULL);
#ifdef __MINGW64_VERSION_MAJOR
  tm = _localtime32(&tv.tv_sec);
#else
  tm = localtime(&tv.tv_sec);
#endif
  strftime(timestamp, sizeof (timestamp), "%Y-%m-%dT%H:%M:%S", tm);
  fprintf(logfile, "[%s.%06d %-5d] ", timestamp, (int) tv.tv_usec, (int) getpid());
#else
  fprintf(logfile, "[%-5d] ", (int)getpid());
#endif
}

/*
 * Write a message to the CCACHE_LOGFILE location (adding a newline).
 */
void cc_log(const char *format, ...)
{
  va_list ap;

  if (!init_log())
  {
    return;
  }

  log_prefix();
  va_start(ap, format);
  vfprintf(logfile, format, ap);
  va_end(ap);
  fprintf(logfile, "\n");
  fflush(logfile);
}

/*
 * Log an executed command to the CCACHE_LOGFILE location.
 */
void cc_log_argv(const char *prefix, char **argv)
{
  if (!init_log())
  {
    return;
  }

  log_prefix();
  fputs(prefix, logfile);
  print_command(logfile, argv);
  fflush(logfile);
}

/* something went badly wrong! */
void fatal(const char *format, ...)
{
  va_list ap;
  char msg[1000];

  va_start(ap, format);
  vsnprintf(msg, sizeof (msg), format, ap);
  va_end(ap);

  cc_log("FATAL: %s", msg);
  fprintf(stderr, "ccache: FATAL: %s\n", msg);

  exit(1);
}

#ifndef HAVE_MKSTEMP
/* cheap and nasty mkstemp replacement */
int mkstemp(char *template)
{
  mktemp(template);
  return open(template, O_RDWR | O_CREAT | O_EXCL | O_BINARY, 0600);
}
#endif

/* make sure a directory exists */
int create_dir(const char *dir)
{
  struct stat st;
  if (stat(dir, &st) == 0)
  {
    if (S_ISDIR(st.st_mode))
    {
      return 0;
    }
    errno = ENOTDIR;
    return 1;
  }
  if (mkdir(dir, 0777) != 0 && errno != EEXIST)
  {
    return 1;
  }
  return 0;
}

/*
 * Return a static string with the current hostname.
 */
const char *get_hostname(void)
{
  static char hostname[200] = "";

  if (!hostname[0])
  {
    strcpy(hostname, "unknown");
#if HAVE_GETHOSTNAME
    gethostname(hostname, sizeof(hostname)-1);
#endif
    hostname[sizeof (hostname) - 1] = 0;
  }

  return hostname;
}

/*
 * Return a string to be used to distinguish temporary files. Also tries to
 * cope with NFS by adding the local hostname.
 */
const char *tmp_string(void)
{
  static char *ret;

  if (!ret)
  {
    ret = format("%s.%u", get_hostname(), (unsigned) getpid());
  }

  return ret;
}

/* Construct a string according to a format. Caller frees. */
char *format(const char *format, ...)
{
  va_list ap;
  char *ptr = NULL;

  va_start(ap, format);
  if (vasprintf(&ptr, format, ap) == -1)
  {
    fatal("Out of memory in format");
  }va_end(ap);

  if (!*ptr)
    fatal("Internal error in format");
  return ptr;
}

/*
 this is like strdup() but dies if the malloc fails
 */
char *x_strdup(const char *s)
{
  char *ret;
  ret = strdup(s);
  if (!ret)
  {
    fatal("Out of memory in x_strdup");
  }
  return ret;
}

/*
 this is like strndup() but dies if the malloc fails
 */
char *x_strndup(const char *s, size_t n)
{
  char *ret;
#ifndef HAVE_STRNDUP
  size_t m;

  if (!s)
  return NULL;
  m = 0;
  while (m < n && s[m])
  {
    m++;
  }
  ret = malloc(m + 1);
  if (ret)
  {
    memcpy(ret, s, m);
    ret[m] = '\0';
  }
#else
  ret = strndup(s, n);
#endif
  if (!ret)
  {
    fatal("x_strndup: Could not allocate %lu bytes", (unsigned long) n);
  }
  return ret;
}

/*
 this is like malloc() but dies if the malloc fails
 */
void *x_malloc(size_t size)
{
  void *ret;
  if (size == 0)
  {
    /*
     * malloc() may return NULL if size is zero, so always do this to make sure
     * that the code handles it regardless of platform.
     */
    return NULL;
  }
  ret = malloc(size);
  if (!ret)
  {
    fatal("x_malloc: Could not allocate %lu bytes", (unsigned long) size);
  }
  return ret;
}

/* This is like calloc() but dies if the allocation fails. */
void *x_calloc(size_t nmemb, size_t size)
{
  void *ret;
  if (nmemb * size == 0)
  {
    /*
     * calloc() may return NULL if nmemb or size is 0, so always do this to
     * make sure that the code handles it regardless of platform.
     */
    return NULL;
  }
  ret = calloc(nmemb, size);
  if (!ret)
  {
    fatal("x_calloc: Could not allocate %lu bytes", (unsigned long) size);
  }
  return ret;
}

/*
 this is like realloc() but dies if the malloc fails
 */
void *x_realloc(void *ptr, size_t size)
{
  void *p2;
  if (!ptr)
    return x_malloc(size);
  p2 = realloc(ptr, size);
  if (!p2)
  {
    fatal("x_realloc: Could not allocate %lu bytes", (unsigned long) size);
  }
  return p2;
}

/*
 * This is like x_asprintf() but frees *ptr if *ptr != NULL.
 */
void x_asprintf2(char **ptr, const char *format, ...)
{
  char *saved = *ptr;
  va_list ap;

  *ptr = NULL;
  va_start(ap, format);
  if (vasprintf(ptr, format, ap) == -1)
  {
    fatal("Out of memory in x_asprintf2");
  }va_end(ap);

  if (!ptr)
    fatal("Out of memory in x_asprintf2");
  if (saved)
  {
    free(saved);
  }
}

/* return the base name of a file - caller frees */
char *basename(const char *s)
{
  char *p;
  p = strrchr(s, '/');
  if (p)
    s = p + 1;
#ifdef _WIN32
  p = strrchr(s, '\\');
  if (p) s = p + 1;
#endif

  return x_strdup(s);
}

/* return the dir name of a file - caller frees */
char *dirname(char *s)
{
  char *p;
  char *p2 = NULL;
  s = x_strdup(s);
  p = strrchr(s, '/');
#ifdef _WIN32
  p2 = strrchr(s, '\\');
#endif
  if (p < p2)
    p = p2;
  if (p)
  {
    *p = 0;
    return s;
  }
  else
  {
    free(s);
    return x_strdup(".");
  }
}

/*
 * Return the file extension (including the dot) of a path as a pointer into
 * path. If path has no file extension, the empty string and the end of path is
 * returned.
 */
const char *get_extension(const char *path)
{
  size_t len = strlen(path);
  const char *p;

  for (p = &path[len - 1]; p >= path; --p)
  {
    if (*p == '.')
    {
      return p;
    }
    if (*p == '/')
    {
      break;
    }
  }
  return &path[len];
}

/*
 * Return a string containing the given path without the filename extension.
 * Caller frees.
 */
char *remove_extension(const char *path)
{
  return x_strndup(path, strlen(path) - strlen(get_extension(path)));
}

/* return size on disk of a file */
size_t file_size(struct stat *st)
{
#ifdef _WIN32
  return (st->st_size + 1023) & ~1023;
#else
  size_t size = st->st_blocks * 512;
  if ((size_t) st->st_size > size)
  {
    /* probably a broken stat() call ... */
    size = (st->st_size + 1023) & ~1023;
  }
  return size;
#endif
}

/* a safe open/create for read-write */
int safe_open(const char *fname)
{
  int fd = open(fname, O_RDWR | O_BINARY);
  if (fd == -1 && errno == ENOENT)
  {
    fd = open(fname, O_RDWR | O_CREAT | O_EXCL | O_BINARY, 0666);
    if (fd == -1 && errno == EEXIST)
    {
      fd = open(fname, O_RDWR | O_BINARY);
    }
  }
  return fd;
}

/* Format a size (in KiB) as a human-readable string. Caller frees. */
char *format_size(size_t v)
{
  char *s;
  if (v >= 1024 * 1024)
  {
    s = format("%.1f Gbytes", v / ((double) (1024 * 1024)));
  }
  else if (v >= 1024)
  {
    s = format("%.1f Mbytes", v / ((double) (1024)));
  }
  else
  {
    s = format("%.0f Kbytes", (double) v);
  }
  return s;
}

/* return a value in multiples of 1024 give a string that can end
 in K, M or G
 */
size_t value_units(const char *s)
{
  char m;
  double v = atof(s);
  m = s[strlen(s) - 1];
  switch (m)
  {
    case 'G':
    case 'g':
    default:
      v *= 1024 * 1024;
      break;
    case 'M':
    case 'm':
      v *= 1024;
      break;
    case 'K':
    case 'k':
      v *= 1;
      break;
  }
  return (size_t) v;
}

#ifndef _WIN32
static long path_max(const char *path)
{
#ifdef PATH_MAX
  (void) path;
  return PATH_MAX;
#elif defined(MAXPATHLEN)
  (void)path;
  return MAXPATHLEN;
#elif defined(_PC_PATH_MAX)
  long maxlen = pathconf(path, _PC_PATH_MAX);
  if (maxlen >= 4096)
  {
    return maxlen;
  }
  else
  {
    return 4096;
  }
#endif
}

/*
 a sane realpath() function, trying to cope with stupid path limits and
 a broken API
 */
char *x_realpath(const char *path)
{
  long maxlen = path_max(path);
  char *ret, *p;

  ret = x_malloc(maxlen);

#if HAVE_REALPATH
  p = realpath(path, ret);
#else
  /* yes, there are such systems. This replacement relies on
   the fact that when we call x_realpath we only care about symlinks */
  {
    int len = readlink(path, ret, maxlen - 1);
    if (len == -1)
    {
      free(ret);
      return NULL;
    }
    ret[len] = 0;
    p = ret;
  }
#endif
  if (p)
  {
    p = x_strdup(p);
    free(ret);
    return p;
  }
  free(ret);
  return NULL;
}
#endif /* !_WIN32 */

/* a getcwd that will returns an allocated buffer */
char *gnu_getcwd(void)
{
  unsigned size = 128;

  while (1)
  {
    char *buffer = (char *) x_malloc(size);
    if (getcwd(buffer, size) == buffer)
    {
      return buffer;
    }
    free(buffer);
    if (errno != ERANGE)
    {
      cc_log("getcwd error: %d (%s)", errno, strerror(errno));
      return NULL;
    }
    size *= 2;
  }
#if defined(__BORLANDC__) || defined(__UPS_ECLIPSE__)
  return 0;
#endif
}

/* create an empty file */
int create_empty_file(const char *fname)
{
  int fd;

  fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC | O_EXCL | O_BINARY, 0666);
  if (fd == -1)
  {
    return -1;
  }
  close(fd);
  return 0;
}

/*
 * Return current user's home directory, or NULL if it can't be determined.
 */
const char *get_home_directory(void)
{
  const char *p = getenv("HOME");
  if (p)
  {
    return p;
  }
#ifdef HAVE_GETPWUID
  {
    struct passwd *pwd = getpwuid(getuid());
    if (pwd)
    {
      return pwd->pw_dir;
    }
  }
#endif
  return NULL;
}

/*
 * Get the current directory by reading $PWD. If $PWD isn't sane, gnu_getcwd()
 * is used. Caller frees.
 */
char *get_cwd(void)
{
  char *pwd;
  char *cwd;
  struct stat st_pwd;
  struct stat st_cwd;

  cwd = gnu_getcwd();
  if (!cwd)
  {
    return NULL;
  }
  pwd = getenv("PWD");
  if (!pwd)
  {
    return cwd;
  }
  if (stat(pwd, &st_pwd) != 0)
  {
    return cwd;
  }
  if (stat(cwd, &st_cwd) != 0)
  {
    return cwd;
  }
  if (st_pwd.st_dev == st_cwd.st_dev && st_pwd.st_ino == st_cwd.st_ino)
  {
    free(cwd);
    return x_strdup(pwd);
  }
  else
  {
    return cwd;
  }
}

/*
 * Check whether s1 and s2 have the same executable name.
 */
bool same_executable_name(const char *s1, const char *s2)
{
#ifdef _WIN32
  bool eq = strcasecmp(s1, s2) == 0;
  if (!eq)
  {
    char *tmp = format("%s.exe", s2);
    eq = strcasecmp(s1, tmp) == 0;
    free(tmp);
  }
  return eq;
#else
  return str_eq(s1, s2);
#endif
}

/*
 * Compute the length of the longest directory path that is common to two
 * strings.
 */
size_t common_dir_prefix_length(const char *s1, const char *s2)
{
  const char *p1 = s1;
  const char *p2 = s2;

  while (*p1 && *p2 && *p1 == *p2)
  {
    ++p1;
    ++p2;
  }
  while (p1 > s1 && ( (*p1 && *p1 != '/') || (*p2 && *p2 != '/')))
  {
    p1--;
    p2--;
  }
  return p1 - s1;
}

/*
 * Compute a relative path from from to to. Caller frees.
 */
char *
get_relative_path(const char *from, const char *to)
{
  size_t common_prefix_len;
  int i;
  const char *p;
  char *result;

  if (!*to || *to != '/')
  {
    return x_strdup(to);
  }

  result = x_strdup("");
  common_prefix_len = common_dir_prefix_length(from, to);
  for (p = from + common_prefix_len; *p; p++)
  {
    if (*p == '/')
    {
      x_asprintf2(&result, "../%s", result);
    }
  }
  if (strlen(to) > common_prefix_len)
  {
    p = to + common_prefix_len + 1;
    while (*p == '/')
    {
      p++;
    }
    x_asprintf2(&result, "%s%s", result, p);
  }
  i = strlen(result) - 1;
  while (i >= 0 && result[i] == '/')
  {
    result[i] = '\0';
    i--;
  }
  if (str_eq(result, ""))
  {
    free(result);
    result = x_strdup(".");
  }
  return result;
}

/*
 * Return whether path is absolute.
 */
bool is_absolute_path(const char *path)
{
#ifdef _WIN32
  return path[0] && path[1] == ':';
#else
  return path[0] == '/';
#endif
}

/*
 * Return whether the argument is a full path.
 */
bool is_full_path(const char *path)
{
  if (strchr(path, '/'))
    return 1;
#ifdef _WIN32
  if (strchr(path, '\\'))
  return 1;
#endif
  return 0;
}

/*
 * Update the modification time of a file in the cache to save it from LRU
 * cleanup.
 */
void update_mtime(const char *path)
{
#ifdef HAVE_UTIMES
  utimes(path, NULL);
#else
  utime(path, NULL);
#endif
}

/*
 * Rename oldpath to newpath (deleting newpath).
 */
int x_rename(const char *oldpath, const char *newpath)
{
#ifdef _WIN32
  /* Windows' rename() refuses to overwrite an existing file. */
  unlink(newpath); /* not x_unlink, as x_unlink calls x_rename */
#endif
  return rename(oldpath, newpath);
}

/*
 * Remove path, NFS hazardous. Use only for temporary files that will not exist
 * on other systems. That is, the path should include tmp_string().
 */
int tmp_unlink(const char *path)
{
  cc_log("Unlink %s (as-tmp)", path);
  return unlink(path);
}

/*
 * Remove path, NFS safe.
 */
int x_unlink(const char *path)
{
  /*
   * If path is on an NFS share, unlink isn't atomic, so we rename to a temp
   * file. We don't care if the temp file is trashed, so it's always safe to
   * unlink it first.
   */
  char* tmp_name = format("%s.%s.rmXXXXXX", path, tmp_string());
  int result = 0;
  cc_log("Unlink %s via %s", path, tmp_name);
  if (x_rename(path, tmp_name) == -1)
  {
    result = -1;
    goto out;
  }
  if (unlink(tmp_name) == -1)
  {
    result = -1;
  }
  out: free(tmp_name);
  return result;
}

#ifndef _WIN32
/* Like readlink() but returns the string or NULL on failure. Caller frees. */
char *
x_readlink(const char *path)
{
  long maxlen = path_max(path);
  ssize_t len;
  char *buf;
#ifdef PATH_MAX
  maxlen = PATH_MAX;
#elif defined(MAXPATHLEN)
  maxlen = MAXPATHLEN;
#elif defined(_PC_PATH_MAX)
  maxlen = pathconf(path, _PC_PATH_MAX);
#endif
  if (maxlen < 4096)
    maxlen = 4096;

  buf = x_malloc(maxlen);
  len = readlink(path, buf, maxlen - 1);
  if (len == -1)
  {
    free(buf);
    return NULL;
  }
  buf[len] = 0;
  return buf;
}
#endif

/*
 * Reads the content of a file. Size hint 0 means no hint. Returns true on
 * success, otherwise false.
 */
bool read_file(const char *path, size_t size_hint, char **data, size_t *size)
{
  int fd, ret;
  size_t pos = 0, allocated;

  if (size_hint == 0)
  {
    struct stat st;
    if (stat(path, &st) == 0)
    {
      size_hint = st.st_size;
    }
  }
  size_hint = (size_hint < 1024) ? 1024 : size_hint;

  fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    return false;
  }
  allocated = size_hint;
  *data = x_malloc(allocated);
  ret = 0;
  while (true)
  {
    if (pos > allocated / 2)
    {
      allocated *= 2;
      *data = x_realloc(*data, allocated);
    }
    ret = read(fd, *data + pos, allocated - pos);
    if (ret == 0 || (ret == -1 && errno != EINTR))
    {
      break;
    }
    if (ret > 0)
    {
      pos += ret;
    }
  }
  close(fd);
  if (ret == -1)
  {
    cc_log("Failed reading %s", path);
    free(*data);
    *data = NULL;
    return false;
  }

  *size = pos;
  return true;
}

/*
 * Return the content (with NUL termination) of a text file, or NULL on error.
 * Caller frees.
 */
char *read_text_file(const char *path)
{
  size_t size;
  char *data;

  if (read_file(path, 0, &data, &size))
  {
    data = x_realloc(data, size + 1);
    data[size] = '\0';
    return data;
  }
  else
  {
    return NULL;
  }
}
