#include <stdio.h>
#include <stdlib.h>

#include "string.h"
#include "config.h"
#include "siparse.h"
#include "utils.h"
#include <errno.h>


int
main(int argc, char *argv[])
{
	line * ln;
	command *com;

	/*ln = parseline("ls -las | grep k | wc ; echo abc > f1 ;  cat < f2 ; echo abc >> f3\n");
	printparsedline(ln);
	printf("\n");
	com = pickfirstcommand(ln);
	printcommand(com,1);
	printf("just before second command\n");
	ln = parseline("sleep 3 &");
	printparsedline(ln);
	printf("\n");
	
	ln = parseline("echo  & abc >> f3\n");
	printparsedline(ln);
	printf("\n");
	com = pickfirstcommand(ln);
	printcommand(com,1);*/


	int continue_flag = 1;
	char buffer[MAX_LINE_LENGTH + 1]; 
	while(continue_flag)
	{
		int bytesRead;
		write(0, PROMPT_STR, sizeof(PROMPT_STR)); // 1. wypisz prompt na std
		bytesRead = read(0, buffer, MAX_LINE_LENGTH); // 2. wczytaj linie z std
		//write(0, buffer, bytesRead);
		if(bytesRead == -1){
			// error handling
			return -1;
		}
		if(bytesRead == 0)
		{
			return 0;
		}
		buffer[bytesRead] = 0;
		ln = parseline(buffer);
		if(!ln) // 4.2 parsowanie zakonczone bledem
		{
			write(0, SYNTAX_ERROR_STR, sizeof(SYNTAX_ERROR_STR)); // 1. wypisz prompt na std
			continue;
		}
		
		pipeline _pipeline = *(ln->pipelines);
		command _command = **_pipeline;
		int childpid = fork();
		if(childpid == 0)
		{ // if kid
			errno = 0;
			execvp(*_command.argv, _command.argv);
			char buffer[MAX_LINE_LENGTH + 33];
			buffer[0] = 0;
			strcat(buffer, *_command.argv);
			if(errno == ENOENT)
			{
				strcat(buffer, ": no such file or directory\n");
			}
			else if(errno == EACCES)
			{
				strcat(buffer, ": permission denied\n");
			}
			else
			{
				strcat(buffer, ": exec error\n");
			}
			write(0, buffer, strlen(buffer));
			
			exit(EXEC_FAILURE);
		}
		
		int result = 0;
		waitpid(-1,&result,0);
		result = WEXITSTATUS(result);
		
		
	}

}
