#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>

#include "builtins.h"
#include "utils.h"
extern char **environ;
int echo(char*[]);
int undefined(char *[]);
int exit_function(char * argv[]);
int lcd_function(char * argv[]);
int lkill_function(char * argv[]);
int lls_function(char * argv[]);
char* findHOME(); // return pointer to text like 'HOME=blah blah blah'
char* findPWD(); // same as findHome
int adjustPWD(char* path);


builtin_pair builtins_table[]={
	{"exit",	&exit_function},
	{"lcd",		&lcd_function},
	{"lkill",	&lkill_function},
	{"lecho",	&echo},
	{"lls",		&lls_function},
	{NULL,NULL} 
};

str_int_pair signal_mapping[]={
	{"SIGQUIT", SIGQUIT},
	{"SIGWINCH", SIGWINCH},
	{"SIGTERM", SIGTERM},
	{"SIGTRAP", SIGTRAP},
	{"SIGTSTP", SIGTSTP},
	{"SIGTTIN", SIGTTIN},
	{"SIGTTOU", SIGTTOU},
	{"SIGUSR1", SIGUSR1},
	{"SIGUSR2", SIGUSR2},
	{"SIGILL", SIGILL},
	{"SIGINT", SIGINT},
	{"SIGPIPE", SIGPIPE},
	{"SIGPROF", SIGPROF},
	{"SIGABRT", SIGABRT},
	{"SIGALRM", SIGALRM},
	{"SIGSEGV", SIGSEGV},
	{"SIGSTKSZ", SIGSTKSZ},
	{"SIGSTOP", SIGSTOP},
	{"SIGFPE", SIGFPE},	
	{"SIGHUP", SIGHUP},
	{"SIGKILL", SIGKILL},
	{"SIGCHLD", SIGCHLD},
	{"SIGCONT", SIGCONT},
	{"SIGVTALRM", SIGVTALRM},	
	{"SIGBUS", SIGBUS},
	//{"SIGSYS", SIGSYS},
	//{"SIGSTKFLT", SIGSTKFLT},	
	//{"SIGCLD", SIGCLD},
	//{"SIGXCPU", SIGXCPU},
	//{"SIGXFSZ", SIGXFSZ},	
	//{"SIGPWR", SIGPWR},	
	//{"SIGRTMAX", SIGRTMAX},
	//{"SIGRTMIN", SIGRTMIN},
	//{"SIGUNUSED", SIGUNUSED},
	//{"SIGURG", SIGURG},
	//{"SIGPOLL", SIGPOLL},
	//{"SIGIO", SIGIO},
	//{"SIGIOT", SIGIOT},	
	{NULL, 0}
};

int echo( char * argv[]){
	int i =1;
	if (argv[i]) printf("%s", argv[i++]);
	while  (argv[i])
		printf(" %s", argv[i++]);
	printf("\n");
	fflush(stdout);
	return 0;
}

int exit_function(char * argv[]){
	int canexit = 1;
	if(argv[1] != 0)
		canexit = 0;
	if(canexit)
		exit(0);
	return -1;
}

int lcd_function(char * argv[]){
	if(argv[1] == 0){
		char buffer[512];
		chdir(findHOME() + strlen("HOME="));
		adjustPWD("");
	} else
	{
		chdir(argv[1]);
		adjustPWD(argv[1]);
	}
	return 0;
}

int find_signal_int_code(char* signal_name, int* signal_number){
	//printf("\n\nlooking for: %s\n\n", signal_name);
	//fflush(stdout);
	int i;
	for(i = 0; signal_mapping[i].name != 0; i++)
	{
		//printf("lookup: %s\n", signal_mapping[i].name);
		//fflush(stdout);
		if(strcmp(signal_name, signal_mapping[i].name) == 0)
		{
			*signal_number = signal_mapping[i].sig;
			return 0;
		}
	}
	return -1;
}

int lkill_error(){
	fprintf(stderr, "Command kill error.\n" );
	return BUILTIN_ERROR;
}
int lkill_function(char * argv[]){
	// get rid of invalid call
	if(argv[1] == 0){
		return lkill_error();
	}
	
	// pid obtaining
	pid_t pid;
	char* pid_text;
	int i;
	for(i = 1; argv[i] != 0; i++){}
	i--;
	if(i > 2)
		return lkill_error();
	pid_text = argv[i];
	
	int converting_result;
	char to_big_buffer[256];
	converting_result = sscanf(pid_text, "%d%s", &pid, &(to_big_buffer[0]));
	if(converting_result != 1){
		return lkill_error();
	}
	
	// signal determining
	// argv[i] points to pid_t which is the last argument
	i--;
	int sig_number = SIGTERM;
	if(i == 1){ // there is custom signal
		converting_result = sscanf(argv[i], "-%d%s", &sig_number, &(to_big_buffer[0]));
		if(converting_result != 1){
			char signal_text[64];
			//sscanf(argv[i]+1, "%s", signal_text);
			//printf("signal text: %s", signal_text);
			fflush(stdout);
			if(find_signal_int_code(&(signal_text[0]), &sig_number) != 0)
			{
				return lkill_error();
			}
		}
	}
	printf("pid: %d\nsig: %d", pid, sig_number);
	fflush(stdout);
	// kill(pid, sig_number);
	return 0;
}

char* findHOME(){
	int i = 0;
	while(environ[i] != 0){
		char* home_env = "HOME";
		if(strncmp(environ[i], home_env, strlen(home_env)) == 0){
			return environ[i];
		}
		i++;
	}
	return NULL;
}

char* findPWD(){
	int i = 0;
	while(environ[i] != 0){
		char* home_env = "PWD";
		if(strncmp(environ[i], home_env, strlen(home_env)) == 0){
			return environ[i];
		}
		i++;
	}
	return NULL;
}

int lls_error(){
	fprintf(stderr, "Builtins lls failed.");
	fflush(stderr);
	return -1;
}

int lls_function(char * argv[]){
	if (argv[1] != 0){
		return lls_error();
	}
	
	DIR* dir;
	dir = opendir(findPWD() + strlen("PWD="));
	
	struct dirent* entry_ptr;
	
	
	while((entry_ptr = readdir(dir)) != 0){
		// TODO: put logic here that prints filenames, ignores one that starts with '.', and take care of the end of the stream
		if(*(entry_ptr->d_name) == '.')
			continue;
		printf("%s\n", entry_ptr->d_name);
		fflush(stdout);
	}
	
	
	
	
	// closedir(dir);
	return 0;
}
/* Adjusting PWD. if path is empty-string then pwd will be equal to home
 * when first char is / then pwd will be set global
 * otherwise path will be appended to the end of pwd
 * 
 * BUG #0001 repeating 'lcd directory' and 'lcd ..' eventualy will cause
 * segfault (I hope)
*/
int adjustPWD(char* path){
	char* pwd_pointer = findPWD();
	char* home_pointer = findHOME();
	if(pwd_pointer == NULL || home_pointer == NULL) // HOME AND PWD MUST EXIST
		return -1;
	if(path[0] == 0){ // home path
		strcpy(pwd_pointer + strlen("PWD="), home_pointer + strlen("HOME="));
	} else if(path[0] == '/'){ // absolute path
		strcpy(pwd_pointer + strlen("PWD="), path);
	} else // relative path
	{
		strcpy(pwd_pointer + strlen(pwd_pointer), "/");
		strcpy(pwd_pointer + strlen(pwd_pointer), path);
	}
	
	return 0;
}


int undefined(char * argv[]){
	fprintf(stderr, "Command %s undefined.\n", argv[0]);
	return BUILTIN_ERROR;
}
