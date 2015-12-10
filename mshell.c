#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#include "string.h"
#include "config.h"
#include "siparse.h"
#include "utils.h"
#include "builtins.h"

#define true 1
#define false 0
#define bool int


#define ChildStatusBuffer 100

#define DEBUG

// handler dla sigchild, trzeba ogarnac ktory z background
// info o backgourndzie przed tuz przed promptem
// zdefiniowac sobie stala buffor dla trzymania informacji o procesach z backgrounda

typedef struct exitStatus{
	pid_t pid;
	int exitTerminatedFlag;
	int terminationCode;
} exitStatus;
exitStatus BGexitStatus[ChildStatusBuffer]; //TODO
int BGexitStatusIndex = 0;
void addBGexitStatus(exitStatus); //TODO
exitStatus removeBGexitStatus(); // return null if empty

pid_t backgroundPID[ChildStatusBuffer];
//


typedef struct redirDetails{
	char *in;
	char *out;
	int inFlag;
	int outFlag;
} redirDetails;
void redirDetailsInit(redirDetails* data);


char linebuf[MAX_LINE_LENGTH + 1];
char buffer[2 * MAX_LINE_LENGTH + 2]; // bufor dla read
char* bufferPtr = buffer;

int countPipelineInLine(line* l);
int countCommandInPipeLine(pipeline* p);

int findEndOfLine(char* tab, int pos, int size); // looking for '\n' char in tab[size] begining from pos
int sinkRead(void);
void say(char* text);
//int checkIfLastChar(char* line, char character);
//int checkIfBgLine(line _line);
redirDetails getRedirDetails(command _command);

void runLine(line* l);
void runPipeLine(pipeline* p);
void runCommand(command _command, int *fd, int hasNext);

int ifEmptyCommand(command* c);


void registerHandlers(void);

int (*(findFunction)(char*))(char **);



// and returning -1 if not found, or x>=pos if found

int
main(int argc, char *argv[]){
	line * ln;
	command *com;
	//saveStreams();
	int printPrompt = 1;
	struct stat statbuffer;

	int continue_flag = 1; // warning: set but not reset
	char* bufferptr = buffer;
	char* lineBegin;
	char* lineEnd;

	int status = fstat(STDIN_FILENO, &statbuffer);
	if (status != 0)
	{// some errors

	}
	else{
		if (!S_ISCHR(statbuffer.st_mode))
			printPrompt = 0;
	}
	while (continue_flag){
		int bytesRead;
		if (printPrompt)
			write(STDOUT_FILENO, PROMPT_STR, sizeof(PROMPT_STR)); // 1. wypisz prompt na std

		//say("Before sinkRead\n");
		int r = sinkRead();
		//say("After sinkRead\n");
		if (r == 0){
			continue_flag = 0;
		}
		//say("Before ParseLine\n");
		ln = parseline(linebuf);
		//say("After parseLine\n");
		if (!ln){
			// 4.2 parsowanie zakonczone bledem
			write(STDOUT_FILENO, SYNTAX_ERROR_STR, sizeof(SYNTAX_ERROR_STR)); // TODO, sprawdz czy stdout czy stderr
			continue;
		}
		
		if(ln != NULL){
			//say("Before runLine\n");
			runLine(ln);
			//say("After runLine\n");
		}
		//ln = NULL;
		//pipeline _pipeline = *(ln->pipelines); // has to be modified to run all commands from line
		//command _command = **_pipeline;
			
		//runCommand(_command); // only one, first command, returns child pid or -1
		//int result = 0;
		//waitpid(-1, &result, 0);
		//result = WEXITSTATUS(result);


	}

}


void runCommand(command _command, int *fd, int hasNext){
	// command from builtins table
	if(findFunction(_command.argv[0]) != NULL){
		int result = (findFunction(_command.argv[0]))(_command.argv); // -1 on error
		if(result == -1){
			char text[MAX_LINE_LENGTH];
			strcpy(text, "Builtin ");
			strcat(text, _command.argv[0]);
			strcat(text, " error.\n");
			write(STDOUT_FILENO, text, strlen(text));
		}
	} else { // regular command
		//int waitForProcess = 1; //!checkIfBgCommand(_command);
		
		/* ******************* */
		/* FORK FORK FORK FORK */
		/* ******************* */
		//say("FORKING\n");
		int childpid = fork(); 
		if (childpid == 0){ // KID
			// switching fd
			if(fd[0] != -1)
			{
				dup2(fd[0], 0); // changing stdin
				//close(fd[0]);
			}
			if(fd[1] != -1 && hasNext)
			{
				dup2(fd[1], 1); // changing stdout
				//close(fd[1]);
			}
			
			// REDIRS handling
			if(_command.redirs != NULL){
				redirDetails details = getRedirDetails(_command);
				if(details.inFlag == 1){
					// look at freopen()
					errno = 0;
					int fd = open(details.in, O_RDONLY);
					int dupresult;
					if(errno == 0) {
						dupresult = dup2(fd ,STDIN_FILENO);
					} else {
						char buffer[128];
						buffer[0] = 0;
						strcat(buffer, details.in);
						if(errno == EACCES) {
							strcat(buffer, ": permission denied\n");
						} else if(errno == ENOENT || errno == ENOTDIR) {
							strcat(buffer, ": no such file or directory\n");
						}
						write(STDERR_FILENO, buffer, strlen(buffer));
						exit(EXEC_FAILURE);
					}
				}
				if(details.outFlag == 2){
					int fd_append = open(details.out, O_WRONLY | O_CREAT | O_APPEND, S_IRWXG);
					dup2(fd_append, STDOUT_FILENO); 
				}
				if(details.outFlag == 1){

					int fd_out = open(details.out, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXG);
					dup2(fd_out, STDOUT_FILENO); 
				}
			} // end redirs handling
	
			errno = 0;
			execvp(*_command.argv, _command.argv);
			
			/* Program shouldn't go below */
			
			char buffer[MAX_LINE_LENGTH + 33];
			buffer[0] = 0;
			strcat(buffer, *_command.argv);
			if (errno == ENOENT){
				strcat(buffer, ": no such file or directory\n");
			}
			else if (errno == EACCES){
				strcat(buffer, ": permission denied\n");
			}
			else{
				strcat(buffer, ": exec error\n");
			}
			write(STDERR_FILENO, buffer, strlen(buffer));
	
			exit(EXEC_FAILURE);
		}
	}
}

int findEndOfLine(char* tab, int pos, int size){
	// looking for '\n' char in tab[size] begining from pos
	// and returning -1 if not found, or x>=pos if found
	int p = pos;
	while (p < size)	{
		if (tab[p] == '\n')
			return p;
		p++;
	}
	return -1;
}


// returns 0 if EOF, else 1
int sinkRead(){
	char tmp[MAX_LINE_LENGTH];
	int bytesRead;
	char* tmpPointer = tmp; // wskazuje pierwsza wolna
	int tmpused = 0;
	while (1){
		errno = 0;
		bytesRead = read(STDIN_FILENO, tmpPointer, MAX_LINE_LENGTH - tmpused); // jezeli bytesread
		if (bytesRead == -1){
			if (errno == EAGAIN)
			{
				say("EAGAIN\n");
				continue;
			}
			return 0;
		}
		if (bytesRead == 0){
			*tmpPointer = '\0';
			memcpy(linebuf, buffer, bufferPtr - buffer);
			memcpy(linebuf + (bufferPtr - buffer), tmp, tmpPointer-tmp); // a co jesli poprzedni odczyt byl niepelny?
			bufferPtr = buffer;
			return 0;
		}
		tmpused += bytesRead;
		tmpPointer += bytesRead;
		int r = findEndOfLine(tmp, 0, tmpused);

		if (r == -1)
			continue;
		memcpy(linebuf, buffer, bufferPtr - buffer);
		memcpy(linebuf + (bufferPtr - buffer), tmp, r+1); // tak samo, a co jesli poprzedni odczyt byl niepelny
		//linebuf[(bufferPtr - buffer) + r + 1] = '\0'; // was
		linebuf[(bufferPtr - buffer) + r] = '\0';
		memcpy(buffer, tmp + r + 1, tmpPointer - tmp - r - 1);

		/*write(STDOUT_FILENO, "line:", 5); // 1. wypisz prompt na std
		write(STDOUT_FILENO, linebuf, MAX_LINE_LENGTH); // 1. wypisz prompt na std
		write(STDOUT_FILENO, "\nbuff: ", 7); // 1. wypisz prompt na std
		write(STDOUT_FILENO, buffer, MAX_LINE_LENGTH); // 1. wypisz prompt na std
		write(STDOUT_FILENO, "\n ", 2); // 1. wypisz prompt na std*/
		
		bufferPtr = buffer + (tmpPointer - tmp) - r - 1;
		return 1;
		break;
	}
	return 0;
}

void say(char* text){
#ifdef DEBUG
	write(STDOUT_FILENO, text, strlen(text)); // 1. wypisz prompt na std
#endif
}

int (*(findFunction)(char* name))(char **) {
	// returns special function that should be executed on main process, or NULL pointer if there is no such function
	if(name == NULL || name[0] == '\0')
		return NULL;

	builtin_pair* pointer = builtins_table;
	while(pointer->name != NULL)
	{
		if(strcmp(pointer->name, name) == 0)
		{
			return pointer->fun;
		}

		pointer++;
	}
	return NULL;
}

redirDetails getRedirDetails(command _command){ 
	// need function to check whether only 1 in and only 1 out
	redirDetails result;
	redirDetailsInit(&result);
	//redirection* pointer = _command.redirs; // tutaj blad, po tym jak po tablicy
	//tutaj blad
	
	int i = 0;
	while(_command.redirs[i] != NULL)
	{
		if(IS_RIN(_command.redirs[i]->flags))
		{
			result.inFlag = 1;
			result.in = _command.redirs[i]->filename;
		}
		if(IS_ROUT(_command.redirs[i]->flags))
		{
			result.outFlag = 1;
			result.out = _command.redirs[i]->filename;
		}

		if(IS_RAPPEND(_command.redirs[i]->flags))
		{
			result.outFlag = 2;
			result.out = _command.redirs[i]->filename;
		}
		i++;
	}
	return result;
}

void redirDetailsInit(redirDetails* data){
	data->inFlag = 0;
	data->outFlag = 0;
	data->in = NULL;
	data->out = NULL;
}


int checkIfLastChar(char* line, char character){
	char* ptr = line;
	while(*(ptr+1)!= 0)
		ptr++;
	return *ptr == character;
}
int checkIfBgLine(line _line){
	return _line.flags == LINBACKGROUND;
/*
	
	if(_command.argv[0] == NULL)
	{	
		say("null odd");
		return 0;
	}
	
	int i = 0;
	while(_command.argv[i+1] != NULL)
	{
	i++;
	}
	return strcmp("&", _command.argv[i]);*/
}


void SIGCHLD_handler(int i){
	int status;
	pid_t p = waitpid(0, &status, 0);
	//pid_t getpgid(pid_t pid); returns proccess group id 
	// it may not work, store background or foreground process id somewhere
}

void registerHandlers( ){
	struct sigaction s;
	s.sa_handler = &SIGCHLD_handler; // functions
	s.sa_sigaction = NULL; // function2, more precise but unnecessery, if flag = something
	s.sa_flags = SA_NOCLDSTOP;
	sigemptyset(&(s.sa_mask));
	
	sigaction(SIGCHLD, &s ,NULL);
}


void runPipeLine(pipeline* p){
	//say("PIPELINE SIGN 402\n"); printf("Number of commands in Pipeline = %i\n", countCommandInPipeLine(p)); fflush(stdout);
	int curpipe[2];
	int prevpipe[2]; 
	prevpipe[0] = prevpipe[1] = -1;
	curpipe[0] = curpipe[1] = -1;
	int i;
	for(i = 0; (*p)[i] != NULL && !ifEmptyCommand((*p)[i]); i++) { // for empty, it shouldnt work
		prevpipe[0] = curpipe[0];
		prevpipe[1] = curpipe[1];
		
		if((*p)[i+1] != NULL) {
			pipe(curpipe);
		} else {
			curpipe[0] = -1;
			curpipe[1] = -1;
		}
		if(prevpipe[1] != -1){
			close(prevpipe[1]);
		}
		//say("PIPELINE SIGN 421\n");	
		int fd[2];
		fd[0] = prevpipe[0];
		fd[1] = curpipe[1];
		//say("PIPELINE SIGN 425\n");			
		runCommand(*((*p)[i]), fd, (*p)[i+1] != NULL);
		//say("PIPELINE SIGN 427\n");
		if(prevpipe[0] != -1) {
			close(prevpipe[0]);
		}
		if(prevpipe[1] != -1) {
			close(prevpipe[1]);
		}
	}
	sleep(1);
	//say("PIPELINE SIGN 436\n");
	while(i-->0) {
		int status;
		waitpid(-1, &status, 0); // not only sigchld may wake it up
		// TODO : implement proper signal handling
	}
	//say("PIPELINE SIGN 442\n");
}

void runLine(line* l){
	//printf("\nprintig line ");
	//printf("Number of pipelines = %i\n", countPipelineInLine(l)); fflush(stdout);
	int i;
	for(i = 0; l->pipelines[i] != NULL; i++)
	{
		//say("before run pipeline");
		runPipeLine(l->pipelines + i);
		//say("after run pipeline");
	}
	fflush(stdout);
}

int countPipelineInLine(line* l){
	int i;
	for(i = 0; (l->pipelines)[i] != NULL; i++){}
	
	return i;
}

int countCommandInPipeLine(pipeline* p){
	int i;
	for(i = 0; (*p)[i] != NULL; i++){}
	
	return i;
}


int ifEmptyCommand(command* c)
{
	return c->argv[0] == 0;
}



