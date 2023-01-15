#ifndef __UTILS_UTILS_H__
#define __UTILS_UTILS_H__

#define STR_MATCH(str1, str2) (strcmp(str1, str2) == 0)

void mem_cleanup(void **ptr);

void gg_free(void *ptr);

void str_cleanup(char **ptr);

void ustr_cleanup(unsigned char **ptr);

int gg_open(const char *path, int flag);

int gg_close(const int fd);

ssize_t gg_read(const int fd, void *buf, size_t count);

void *gg_calloc(size_t n, size_t size);

#endif // __UTILS_UTILS_H__