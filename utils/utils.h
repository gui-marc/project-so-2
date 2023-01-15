#ifndef __UTILS_UTILS_H__
#define __UTILS_UTILS_H__

#define STR_MATCH(str1, str2) (strcmp(str1, str2) == 0)

void mem_cleanup(void **ptr);

void str_cleanup(char **ptr);

void ustr_cleanup(unsigned char **ptr);

#endif // __UTILS_UTILS_H__