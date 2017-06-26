#ifndef PCACHE_UTIL_H
#define PCACHE_UTIL_H

int string_match_len(const char *p, int plen, const char *s, int slen, int nocase);

void pcache_atoi(const char *str, int *ret, int *len);

#endif
