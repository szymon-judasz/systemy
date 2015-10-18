#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "string.h"
#include "config.h"
#include "siparse.h"
#include "utils.h"


char line[MAX_LINE_LENGTH + 1];
char buffer[2 * MAX_LINE_LENGTH + 2]; // bufor dla read
char* bufferPtr = buffer;

void runCommand(command _command);
int findEndOfLine(char* tab, int pos, int size); // looking for '\n' char in tab[size] begining from pos
int sinkRead(void);
// and returning -1 if not found, or x>=pos if found

int
main(int argc, char *argv[])
{
	line * ln;
	command *com;

	int printPrompt = 1;
	struct stat statbuffer;

	int continue_flag = 1; // warning: set but not reset
	char* bufferptr = buffer;
	char* lineBegin;
	char* lineEnd;
	char line[MAX_LINE_LENGTH];

	int status = fstat(STDIN_FILENO, &statbuffer);
	if (status != 0)
	{// some errors

	}
	else
	{
		if (!S_ISCHR(statbuffer.st_mode))
			printPrompt = 0;
	}
	while (continue_flag)
	{
		int bytesRead;
		if (printPrompt)
			write(STDOUT_FILENO, PROMPT_STR, sizeof(PROMPT_STR)); // 1. wypisz prompt na std

		bool continue_flag = true;
		while (continue_flag)
		{
			int r = sinkRead();
			if (r == 0)
			{
				continue_flag = false;
			}

			ln = parseline(line);
			if (!ln) // 4.2 parsowanie zakonczone bledem
			{
				write(STDOUT_FILENO, SYNTAX_ERROR_STR, sizeof(SYNTAX_ERROR_STR)); // 1. wypisz prompt na std
				continue;
			}

			pipeline _pipeline = *(ln->pipelines);
			command _command = **_pipeline;

			runCommand(_command);


			int result = 0;
			waitpid(-1, &result, 0);
			result = WEXITSTATUS(result);
		}

	}

}

void runCommand(command _command)
{
	int childpid = fork();
	if (childpid == 0)
	{ // if kid
		errno = 0;
		execvp(*_command.argv, _command.argv);
		char buffer[MAX_LINE_LENGTH + 33];
		buffer[0] = 0;
		strcat(buffer, *_command.argv);
		if (errno == ENOENT)
		{
			strcat(buffer, ": no such file or directory\n");
		}
		else if (errno == EACCES)
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
	while (p < size)
	{
		if (tab[p] == '\n')
			return p;
		p++;
	}
	return -1;
}


// returns 0 if EOF, else 1
int sinkRead()
{
	char tmp[MAX_LINE_LENGTH];
	int bytesRead;
	char* tmpPointer = tmp; // wskazuje pierwsza wolna
	int tmpused = 0;
	while (true)
	{
		errno = 0;
		bytesRead = read(STDIN_FILENO, tmpPointer, MAX_LINE_LENGTH - tmpused); // jezeli bytesread
		if (bytesRead == -1)
		{
			if (errno == EAGAIN)
			{
				continue;
			}
			return 0;
		}
		if (bytesRead == 0)
		{
			*tmpPointer = '\0';
			memcpy(line, buffer, bufferPtr - buffer);
			memcpy(line + bufferPtr - buffer, tmp, tmpPointer-tmp); // a co jesli poprzedni odczyt byl niepelny?
			bufferPtr = buffer;
			return 0;
		}
		tmpused += bytesRead;
		tmpPointer += bytesRead;
		int r = findEndOfLine(tmp, 0, tmpused);
		if (r == -1)
			continue;
		memcpy(line, buffer, bufferPtr - buffer);
		memcpy(line + bufferPtr - buffer, tmp, r+1); // tak samo, a co jesli poprzedni odczyt byl niepelny
		memcpy(buffer, tmp + r + 1, tmpPointer - tmp - r - 1);
		bufferPtr = buffer + tmpPointer - tmp - r - 1;
		return 1;
		break;
	}
	return 0;
}