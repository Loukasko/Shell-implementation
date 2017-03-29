#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#define delim " \t\r\n\a"
#define read_end 0
#define write_end 1

struct process{

	char ** argv;
	struct process *nextprocess ;
};

struct job{
	struct process *first_process;
};


void loop();
char * readTheLine();
struct job split_line(char *line);
int initialize(struct job *job);
void fork_exec(char **argv,int fd_in, int fd_out);

static int bufsize=1024;
int numpipes=0;


struct job job;

int main(void) {
	loop();
	return EXIT_SUCCESS;
}


void loop(){
	char *line;
	char antio[10];
	int status;
	char cd[3];
	char pr[100];
	strcpy(cd,"cd");

	strcpy(antio,"exit\n");
	do{
		printf("$ ");
		line=readTheLine();
		if ( strcmp(line,antio) ==0 ){
			exit(1);
		}
		job=split_line(line);
		if (strcmp(job.first_process->argv[0],cd)==0){
			strcpy(pr,job.first_process->argv[1]);
			status=chdir(pr);
			numpipes=0;
			job.first_process=NULL;
			status=1;
			continue;
		}
		status=initialize(&job);
		free(line);
	}while (status);
}

char * readTheLine(){
	char *line =NULL;
	getline(&line,&bufsize,stdin);
	return line;
}

struct job split_line(char *line){


	int buf=bufsize;
	char **tokens=malloc(buf * sizeof(char*));
	char *token;
	int pos=0;
	char pipe[2];
	struct job job;
	struct process *process=malloc(sizeof(struct process));
	strcpy(pipe,"|");

	if(!tokens){
		exit(EXIT_FAILURE);
	}

	token=strtok(line,delim);
	do{
		tokens[pos]=token;
		pos++;

		if (pos>=buf){
			buf+=bufsize;
			tokens=realloc(tokens,bufsize*sizeof(char*));
			if(!tokens){
				exit(EXIT_FAILURE);
			}
		}

		token=strtok(NULL,delim);
		if (token==NULL){
			process->argv=tokens;
			if(numpipes==0){
				job.first_process=process;
				process->nextprocess=NULL;
			}else {
				struct process *help=malloc(sizeof(struct process));
				help=job.first_process;
				while(help->nextprocess!=NULL){
					help=help->nextprocess;
					if(help->nextprocess==NULL){
						help=process;
					}
				}
				help=NULL;
			}
			tokens[pos]=NULL;
			return job;
		}



		if (strcmp(token,pipe)==0){
			numpipes++;
			process->argv=tokens;
			tokens=malloc(buf * sizeof(char*));
			pos=0;
			if (numpipes==1){
				job.first_process=process;
				process=malloc(sizeof(struct process));
				job.first_process->nextprocess=process;

			}
			else{
				struct process *help=malloc(sizeof(struct process));
				help=job.first_process;
				while(help->nextprocess!=NULL){
					help=help->nextprocess;
					if(help->nextprocess==NULL){
						process=malloc(sizeof(struct process));
						help->nextprocess=process;
						break;
					}
				}
				help=NULL;
			}
			token=strtok(NULL,delim);
		}

	}while(token!=NULL);
	return job;
}
void fork_exec(char **argv,int fd_in, int fd_out){
	pid_t child=fork();
	int status;
	if (child){
		waitpid(child,&status,0);
		return ;
	}
	if (fd_in!=-1 && fd_in!=0){
		dup2(fd_in,0);
		close(fd_in);
	}
	if (fd_out!=-1 && fd_in !=1){
		dup2(fd_out,1);
		close(fd_out);
	}
	execvp(argv[0],argv);
	exit(-1);

}


int initialize(struct job *job){
	struct process *help=malloc(sizeof(struct process));
	help=job->first_process;
	int fd_in=-1;
	int fd_out;
	int i=0;

	while((help)&&(i!=1)){
		int fd_pipe[2];
		if (help->nextprocess!=NULL){
			pipe(fd_pipe);
			fd_out=fd_pipe[1];
		}
		else{
			fd_out=-1;
		}

		fork_exec(help->argv,fd_in,fd_out);

		if(!help->nextprocess){
			break;
		}
		help=help->nextprocess;
		close(fd_in);
		close(fd_out);
		fd_in=fd_pipe[0];
	}

	numpipes=0;
	job->first_process=NULL;
	return 1;
}

