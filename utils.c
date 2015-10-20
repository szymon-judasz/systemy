#include <stdio.h>

#include "config.h"
#include "siparse.h"

void 
printcommand(command *pcmd, int k)
{
	char ** arg;
	redirection ** redirs;
	int flags;

	printf("\tCOMMAND %d\n",k);
	if (pcmd==NULL){
		printf("\t\t(NULL)\n");
		return;
	}

	printf("\t\targv=:");
	for (arg=pcmd->argv;*arg;arg++){
		printf("%s:",*arg);
	}

	printf("\n\t\tredirections=:");
	for (redirs=pcmd->redirs;*redirs;redirs++){
		flags = (*redirs)->flags;
		printf("(%s,%s):",(*redirs)->filename,IS_RIN(flags)?"<": IS_ROUT(flags) ?">": IS_RAPPEND(flags)?">>":"??");
	}

	printf("\n");
}

void
printpipeline(pipeline p, int k)
{
	int c;
	command ** pcmd;

	printf("PIPELINE %d\n",k);
	if (p==NULL){
		printf("\t(NULL)\n");
		return;
	}

	c=1;
	for (pcmd=p;*pcmd;pcmd++,c++){
		printcommand(*pcmd,c);
	}
	printf("Totally %d commands in pipeline %d.\n",c-1,k);
}

void
printparsedline(line * ln)
{
	int c;
	pipeline * p;

	if (!ln){
		printf("%s\n",SYNTAX_ERROR_STR);
		return;
	}
	c=1;
	for (p=ln->pipelines;*p;p++, c++){
		printpipeline(*p,c);
	}
	printf("Totally %d pipelines.",c-1);
	if (ln->flags&LINBACKGROUND){
		printf(" Line in background.");
	}
	printf("\n");
}

command *
pickfirstcommand(line * ln)
{
	if  ( (ln==NULL)
		||(*(ln->pipelines)==NULL) 
		||(**(ln->pipelines)==NULL)
		)	return NULL;

	return **(ln->pipelines);
}

int cstringtoint(char* str, int* result)
{
	if(str == NULL || *str == 0)
	{
		return -1;
	}
	int temp = 0;
	char* ptr = str;
	while(*ptr != 0)
	{
		if(*ptr < '0' || *ptr > '9')
			return -1;
		temp *= 10;
		temp += *ptr - '0';
		ptr++;
	}
	*result = temp;
	return 1;
}


