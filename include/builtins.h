#ifndef _BUILTINS_H_
#define _BUILTINS_H_

#define BUILTIN_ERROR 2

typedef struct {
	char* name;
	int (*fun)(char**);
} builtin_pair;

extern builtin_pair builtins_table[];

typedef struct {
	char* name;
	int sig;
} str_int_pair;
#endif /* !_BUILTINS_H_ */
