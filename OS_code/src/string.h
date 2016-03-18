#ifndef STRING_H_
#define STRING_H_

extern int strcmp(char *s1, char *s2);
extern void strcpy(char *src, char *dest);
extern int is_prefix(char *pre, char *test);
extern int is_digit(char c);
extern int is_substring(char *sub, char *test);
	
#endif
