//====================================================================================
// mostly taken from ccache util.c
//====================================================================================

#ifndef UTIL_H
#define UTIL_H

#include <sys/stat.h>

#include "chameleon.h"

void cc_log(const char *format, ...) ATTR_FORMAT(printf, 1, 2);
void cc_log_argv(const char *prefix, char **argv);
void fatal(const char *format, ...) ATTR_FORMAT(printf, 1, 2);

void copy_fd(int fd_in, int fd_out);
int copy_file(const char *src, const char *dest, int compress_dest);
int move_file(const char *src, const char *dest, int compress_dest);
int move_uncompressed_file(const char *src, const char *dest,
                           int compress_dest);
bool file_is_compressed(const char *filename);

int create_dir(const char *dir);
const char *get_hostname(void);
const char *tmp_string(void);
char *format_hash_as_string(const unsigned char *hash, unsigned size);
int create_hash_dir(char **dir, const char *hash, const char *cache_dir);
int create_cachedirtag(const char *dir);
char *format(const char *format, ...) ATTR_FORMAT(printf, 1, 2);
char *x_strdup(const char *s);
char *x_strndup(const char *s, size_t n);
void *x_realloc(void *ptr, size_t size);
void *x_malloc(size_t size);
void *x_calloc(size_t nmemb, size_t size);
void traverse(const char *dir, void (*fn)(const char *, struct stat *));
char *basename(const char *s);
char *dirname(char *s);
const char *get_extension(const char *path);
char *remove_extension(const char *path);
size_t file_size(struct stat *st);
int safe_open(const char *fname);
char *x_realpath(const char *path);
char *gnu_getcwd(void);
int create_empty_file(const char *fname);
const char *get_home_directory(void);
char *get_cwd();
bool same_executable_name(const char *s1, const char *s2);
size_t common_dir_prefix_length(const char *s1, const char *s2);
char *get_relative_path(const char *from, const char *to);
bool is_absolute_path(const char *path);
bool is_full_path(const char *path);
void update_mtime(const char *path);
int x_rename(const char *oldpath, const char *newpath);
int x_unlink(const char *path);
int tmp_unlink(const char *path);
char *x_readlink(const char *path);
char *read_text_file(const char *path);
bool read_file(const char *path, size_t size_hint, char **data, size_t *size);

#endif // UTIL_H

