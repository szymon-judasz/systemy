#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "builtins.h"
#include "utils.h"

int echo(char*[]);
int undefined(char *[]);
int exit_function(char * argv[]);
//int lcd_function(char * argv[]);

builtin_pair builtins_table[]={
	{"exit",	&exit_function},
	{"lecho",	&echo},
	{"lcd",		&undefined},
	{"lkill",	&undefined},
	{"lls",		&undefined},
	{NULL,NULL} // tabela musi sie konczyc dwoma nullami,
};
// to ma dzialac przez iteracje po tabeli

int 
echo( char * argv[])
{
	int i =1;
	if (argv[i]) printf("%s", argv[i++]);
	while  (argv[i])
		printf(" %s", argv[i++]);

	printf("\n");
	fflush(stdout);
	return 0;
}

int exit_function(char * argv[])
{
	int canexit = 1;
	if(argv[1] != 0)
		canexit = 0;
	if(canexit)
		exit(0);
	return -1;


	// wrong, exit doesn't take parameters
	/*int canexit = 1;

	int codeofexit;
	if(cstringtoint(argv[1], &codeofexit) == -1)
		canexit 0;
	if(canexit)
		return exit(codeofexit);
	return -1;*/
}

//int lcd_function(char * argv[])

int 
undefined(char * argv[])
{
	fprintf(stderr, "Command %s undefined.\n", argv[0]);
	return BUILTIN_ERROR;
}
