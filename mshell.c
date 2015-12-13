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
#include "include/siparse.h"
#include "include/builtins.h"

// handler dla sigchild, trzeba ogarnac ktory z background
// info o backgourndzie przed tuz przed promptem
// zdefiniowac sobie stala buffor dla trzymania informacji o procesach z backgrounda

typedef struct spawnedProcessData{
	pid_t pid;
	int exitNotTerminated;
	int exitTerminationCode;
	int hasRunInBG;
	int stillRuning;
} spawnedProcessData;

int amountSpawnedProcess = 0;
spawnedProcessData BGexitStatus[ChildStatusBuffer]; //TODO
int findProcessData(pid_t pid);
void addProcessData(struct spawnedProcessData pData);
void removeProcessData(pid_t pid);
void askForChildStatus();
void printChildStatus();


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

int findEndOfLine(char* tab, int pos, int size);
int sinkRead(void);

redirDetails getRedirDetails(command _command);

void runLineInBG(line *l);

void runLine(line* l);
void runBGLine(line* line);
void runPipeLine(pipeline* p);
int runCommand(command _command, int *fd, int hasNext); // returns 1 when child was spawned, otherwise 0 eg. number of spawned processes

int ifEmptyCommand(command* c);


void registerHandlers(void);

int (*(findFunction)(char*))(char **);



// and returning -1 if not found, or x>=pos if found

int
main(int argc, char *argv[]){
	line * ln;
	command *com;
	int printPrompt = 1;
	struct stat statbuffer;

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
	while (1){
		int bytesRead;
		askForChildStatus();
		if (printPrompt)
		{
			printChildStatus();
			write(1, PROMPT_STR, sizeof(PROMPT_STR)); // 1. wypisz prompt na std
		}


		int r = sinkRead();
		if (r == 0){
			break;
		}
		ln = parseline(linebuf);
		if (!ln){
			write(STDERR_FILENO, SYNTAX_ERROR_STR, sizeof(SYNTAX_ERROR_STR)); // TODO, sprawdz czy stdout czy stderr
			continue;
		}
		if (ln->flags != LINBACKGROUND)
		{
			runLine(ln);
		} else
		{
			runBGLine(ln);
		}

	}

}


int runCommand(command _command, int *fd, int hasNext){
	// command from builtins table
	if(findFunction(_command.argv[0]) != NULL){
		int result = (findFunction(_command.argv[0]))(_command.argv); // -1 on error
		if(result == -1){
			char text[MAX_LINE_LENGTH];
			strcpy(text, "Builtin ");
			strcat(text, _command.argv[0]);
			strcat(text, " error.\n");
			write(1, text, strlen(text));
		}
		return 0;
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
				close(fd[0]);
			}
			if(fd[1] != -1 && hasNext)
			{
				dup2(fd[1], 1); // changing stdout
				close(fd[1]);
			}
			
			// REDIRS handling
			if(_command.redirs != NULL){
				redirDetails details = getRedirDetails(_command);
				if(details.inFlag == 1){
					// look at freopen()
					errno = 0;
					int fd_open;
					fd_open = open(details.in, O_RDONLY);
					//void* fd;
					//fd = freopen(details.in, "r", stdin);
					//int dupresult;
					if(errno == 0 /*&& fd != 0*/) {
						dup2(fd_open ,0);
						close(fd_open);
					} else {
						char buffer[128];
						buffer[0] = 0;
						strcat(buffer, details.in);
						if(errno == EACCES) {
							strcat(buffer, ": permission denied\n");
						} else if(errno == ENOENT || errno == ENOTDIR) {
							strcat(buffer, ": no such file or directory\n");
						}
						write(2, buffer, strlen(buffer));
						exit(EXEC_FAILURE);
					}
				}
				if(details.outFlag == 2){
					int fd_append = open(details.out, O_WRONLY | O_CREAT | O_APPEND, S_IRWXG);
					dup2(fd_append, 1); 
					close(fd_append);
				}
				if(details.outFlag == 1){
					int fd_out = open(details.out, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXG);
					dup2(fd_out, 1);
					close(fd_out);
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
			write(2, buffer, strlen(buffer));
	
			exit(EXEC_FAILURE);
		}
		// parent
		struct spawnedProcessData spawned;
		spawned.pid = childpid;
		spawned.hasRunInBG = 0;
		spawned.stillRuning = 1;
		void addProcessData(spawned);
	}
	return 1;
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


// returns 0 if EOF, -1 if signal, 1 else
int sinkRead(){
	int pos = 0; // pozycja w buforze lini
	
	while(1){
		char buff;
		int status;
		errno = 0;
		status = read(STDIN_FILENO, &buff, 1);
		if(status == -1){
			if (errno == EAGAIN || errno == EINTR){
				continue;
			} else {
				return -1;
			}
		}
		if(status == 0){
			linebuf[pos++] = '\n';
			linebuf[pos] = 0;
			return 0;
		}
		if(buff == '\n'){
			linebuf[pos++] = buff;
			linebuf[pos] = 0;
			return 1;
		}
		linebuf[pos++] = buff;
		if(pos >= MAX_LINE_LENGTH)
		{
			write(2, SYNTAX_ERROR_STR, strlen(SYNTAX_ERROR_STR)); // TODO, sprawdz czy stdout czy stderr
			linebuf[0] = '\n';
			linebuf[1] = 0;
			while(1){
				errno = 0;
				status = read(STDIN_FILENO, &buff, 1);
				if(status == -1){
					if(errno == EAGAIN){
						continue;
					} else {
						return -1;
					}
				}
				if(status == 0){
					linebuf[0] = '\n';
					linebuf[1] = 0;
					return 0;
				}
				if(buff == '\n'){
					linebuf[0] = '\n';
					linebuf[1] = 0;
					return 1;
				}
			}

		}
	}
	return -1;
	}

int (*(findFunction)(char* name))(char **) {
	if(name == NULL || name[0] == '\0')
		return NULL;
	builtin_pair* pointer = builtins_table;
	while(pointer->name != NULL) {
		if(strcmp(pointer->name, name) == 0) {
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
	
	int i = 0;
	while(_command.redirs[i] != NULL){
		if(IS_RIN(_command.redirs[i]->flags)){
			result.inFlag = 1;
			result.in = _command.redirs[i]->filename;
		}
		if(IS_ROUT(_command.redirs[i]->flags)){
			result.outFlag = 1;
			result.out = _command.redirs[i]->filename;
		}
		if(IS_RAPPEND(_command.redirs[i]->flags)){
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
	int curpipe[2];
	int prevpipe[2]; 
	prevpipe[0] = prevpipe[1] = -1;
	curpipe[0] = curpipe[1] = -1;
	int i;
	int spawnedProccesses = 0;
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
		int fd[2];
		fd[0] = prevpipe[0];
		fd[1] = curpipe[1];
		spawnedProccesses += runCommand(*((*p)[i]), fd, (*p)[i+1] != NULL);
		if(prevpipe[0] != -1) {
			close(prevpipe[0]);
		}
		if(prevpipe[1] != -1) {
			close(prevpipe[1]);
		}
	}
	int terminatedSpawnedChilds = 0;
	while (spawnedProccesses>terminatedSpawnedChilds) {
		int status;
		pid_t pid = 0;
		pid = waitpid(-1, &status, 0); // not only sigchld may wake it up
		if(pid == 0 || pid == -1)
		{
			continue;
		}
		int pos = findProcessData(pid);
		if (pos == -1)
			continue;
		struct spawnedProcessData* pData = &(BGexitStatus[pos]);
		if (pData->hasRunInBG == 0)
		{
			removeProcessData(pos);
			terminatedSpawnedChilds++;
		}
		else
		{
			if (WIFEXITED(status))
			{
				pData->exitNotTerminated = 1;
				pData->exitTerminationCode = WEXITSTATUS(status);
			} else
			{
				pData->exitNotTerminated = 0;
				pData->exitTerminationCode = WTERMSIG(status);
			}
			pData->stillRuning = 0;
		}
	}
	//say("PIPELINE SIGN 442\n");
}

void runLine(line* l){
	int i;
	for(i = 0; l->pipelines[i] != NULL; i++){
		runPipeLine(l->pipelines + i);
	}
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

int ifEmptyCommand(command* c){
	return c->argv[0] == 0;
}

int findProcessData(pid_t pid){
	int i;
	for (i = 0; i < amountSpawnedProcess; i++){
		if (BGexitStatus[i].pid == pid){
			return i;
		}
	}
	return -1;
}
void addProcessData(struct spawnedProcessData pData){
	BGexitStatus[amountSpawnedProcess++] = pData;
}
void removeProcessData(pid_t pid){
	int pos = findProcessData(pid);
	if (pos == -1){
		return;
	}
	if (pos + 1 == amountSpawnedProcess){
		amountSpawnedProcess--;
		return;
	}
	BGexitStatus[pos] = BGexitStatus[--amountSpawnedProcess];
	return;
}
void askForChildStatus(){
	pid_t pid;
	int status;
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		int pos = findProcessData(pid);
		if (pos == -1)
			break;
		struct spawnedProcessData* pData = &(BGexitStatus[pos]);
		if (WIFEXITED(status))
		{
			pData->exitNotTerminated = 1;
			pData->exitTerminationCode = WEXITSTATUS(status);
		}
		else
		{
			pData->exitNotTerminated = 0;
			pData->exitTerminationCode = WTERMSIG(status);
		}
		pData->stillRuning = 0;
	}

}
void printChildStatus(){
	int index = amountSpawnedProcess - 1;
	while (index >= 0)
	{
		if (BGexitStatus[index].stillRuning == 0)
		{
			char txtbuffer[100];
			sprintf(txtbuffer, "Background process %i terminated. ", BGexitStatus[index].pid);
			int len = strlen(txtbuffer);
			if (BGexitStatus[index].exitNotTerminated == 1)
			{
				sprintf(txtbuffer + len, "(exited with status %i)\n", BGexitStatus[index].exitTerminationCode);
			}
			else
			{
				sprintf(txtbuffer + len, "(killed by signal %i)\n", BGexitStatus[index].exitTerminationCode);
			}
			write(1, txtbuffer, strlen(txtbuffer));
			removeProcessData(BGexitStatus[index].pid);
		}
		index--;
	}
}

runBGLine(line* ln)
{
	pipeline *p = ln->pipelines[0];

}