#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "string.h"
#include "config.h"
#include "siparse.h"
#include "utils.h"


void runCommand(command _command);
int findEndOfLine(char* tab, int pos, int size); // looking for '\n' char in tab[size] begining from pos
// and returning -1 if not found, or x>=pos if found

int
main(int argc, char *argv[])
{
	line * ln;
	command *com;

	int printPrompt = 1;
	struct stat statbuffer;
	
	int continue_flag = 1; // warning: set but not reset
	char buffer[2 * MAX_LINE_LENGTH + 1]; // bufor dla read
	char* bufferptr = buffer;
	char* lineBegin;
	char* lineEnd;
	char line[MAX_LINE_LENGTH];

	int status = fstat(STDIN_FILENO, &statbuffer);
	if(status!=0)
	{// some errors
		
	} else
	{
		if(!S_ISCHR(statbuffer.st_mode))
			printPrompt = 0;
	}
	while(continue_flag)
	{
		int bytesRead;
		if(printPrompt)
			write(STDOUT_FILENO, PROMPT_STR, sizeof(PROMPT_STR)); // 1. wypisz prompt na std
		
		bufferptr = buffer;
		bytesRead = read(STDIN_FILENO, buffer, MAX_LINE_LENGTH); // 2. wczytaj linie z std
		//write(0, buffer, bytesRead);
		if(bytesRead == -1)	return -1;
		if(bytesRead == 0) return 0;
		
		//bufferptr += bytesRead;
		
		int lastPos = 0;
		while(lastPos != 0)
		{
			lineBegin = bufferptr;
			lastPos = findEndOfLine(buffer, bufferptr - buffer, MAX_LINE_LENGTH);
			buffer[lastPos] = 0;
			bufferptr = buffer + lastPos + 1;
			
			
		
		
			//buffer[bytesRead] = 0;
			ln = parseline(lineBegin);
			if(!ln) // 4.2 parsowanie zakonczone bledem
			{
				write(STDOUT_FILENO, SYNTAX_ERROR_STR, sizeof(SYNTAX_ERROR_STR)); // 1. wypisz prompt na std
				continue;
			}
		
			pipeline _pipeline = *(ln->pipelines);
			command _command = **_pipeline;
		
			runCommand(_command);
		
		
			int result = 0;
			waitpid(-1,&result,0);
			result = WEXITSTATUS(result);
		}
		
	}

}

void runCommand(command _command)
{
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
			write(STDERR_FILENO, buffer, strlen(buffer));
			
			exit(EXEC_FAILURE);
		}
}

int findEndOfLine(char* tab, int pos, int size) // looking for '\n' char in tab[size] begining from pos
{	// and returning -1 if not found, or x>=pos if found
	int p = pos;
	while(p<size)
	{
		if(tab[p] == '\n')
			return p;
		p++;
	}
	return -1;
}
