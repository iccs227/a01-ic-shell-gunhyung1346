/* ICCS227: Project 1: icsh
 * Name: GunHyung Kim
 * StudentID: 6480233
 */
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <signal.h>
 #include <unistd.h>
 #include <sys/wait.h>
 #include <fcntl.h>
 #include <limits.h>

#define MAX_CMD_BUFFER 255
typedef enum {RUNNING, STOPPED} JobStatus;

typedef struct Job{
	int job_id;
	pid_t pid;
	char command[MAX_CMD_BUFFER];
	JobStatus status;
	struct Job *next;
} Job;

Job* job_list = NULL;
int next_jid = 1;
pid_t foreground_pid = -1;
int last_exit_status = 0;

int add_job(pid_t pid, const char* command, JobStatus status){
	Job* new_job = malloc(sizeof(Job));
	new_job->job_id = next_jid++;
	new_job->pid = pid;
	new_job->status = status;
	strncpy(new_job->command, command, MAX_CMD_BUFFER);
	new_job->command[MAX_CMD_BUFFER-1] = '\0';
	
	if(job_list==NULL||new_job->job_id < job_list->job_id){
		new_job->next = job_list;
		job_list = new_job;
		return new_job->job_id;
	}
	Job* current = job_list;
	while(current->next!=NULL && current->next->job_id < new_job->job_id){
		current = current->next;
	}
	new_job->next = current->next;
	current->next = new_job;
	return new_job->job_id;
}

void remove_job(pid_t pid){
	Job *prev = NULL;
	Job *curr = job_list;
	while(curr){
		if(curr->pid == pid){
			if (prev){
				prev->next = curr->next;
			}else{
				job_list = curr->next;
			}
			free(curr);
			return;
		}
		prev=curr;
		curr = curr->next;
	} 
}

void print_jobs(){
	Job* job = job_list;
	while(job){
		printf("[%d]  %s\t%s\n", job->job_id, job->status == RUNNING ? "Running" : "Stopped", job->command);
		job = job->next;
	}
}

Job* get(int job_id){
	Job* job = job_list;
	while(job){
		if (job->job_id==job_id){
			return job;
		}
		job = job->next;
	}
	return NULL;
}

void free_jobs(){
	Job* curr = job_list;
	while(curr){
		Job* temp = curr;
		curr = curr->next;
		free(temp);
	}
	job_list = NULL;
}

void sigint_handler(int sig, siginfo_t *si, void *notused){
	if(foreground_pid>0){
		kill(foreground_pid,SIGINT);
		printf("\n");
		fflush(stdout);
	}
}

void sigtstp_handler(int sig, siginfo_t *si, void *notused){
	if(foreground_pid>0){
		kill(foreground_pid,SIGTSTP);
		printf("\n");
		fflush(stdout);
	}
}

void sigchld_handler(int sig, siginfo_t *si, void *notused){
	pid_t pid;
	int status;
	while((pid=waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED))>0){
		Job* job = job_list;
		while(job){
			if(job->pid==pid){
				break;
			}
			job = job->next;
		}
		if(!job){
			continue;
		}
		if(WIFEXITED(status) || WIFSIGNALED(status)){
			if(job->status == RUNNING){
				printf("\n[%d]+  Done \t\t%s\n",job->job_id,job->command);
				fflush(stdout);
				remove_job(pid);
			}
		}else if(WIFSTOPPED(status)){
			job->status = STOPPED;
			printf("\n[%d]+  Stopped\t\t%s\n", job->job_id, job->command);
			fflush(stdout);
		}else if(WIFCONTINUED(status)){
			job->status = RUNNING;
		}
	}
}

void prompt(){
    printf("icsh $ ");
    fflush(stdout);
}

void newline(char *str){
    str[strcspn(str, "\n")] = 0;
}

int main(int argc, char *argv[]) {
	if(next_jid == INT_MAX){
		next_jid = 1;
	}
    char buffer[MAX_CMD_BUFFER];
    char last_cmd[MAX_CMD_BUFFER] = "";
    FILE *input = stdin;
	int is_script = 0;

	struct sigaction sa_int, sa_tstp, sa_chld;
	sa_int.sa_sigaction = sigint_handler;
	sigfillset(&sa_int.sa_mask);
	sa_int.sa_flags = SA_SIGINFO;
	sigaction(SIGINT,&sa_int,NULL);

	sa_tstp.sa_sigaction = sigtstp_handler;
	sigfillset(&sa_tstp.sa_mask);
	sa_tstp.sa_flags = SA_SIGINFO;
	sigaction(SIGTSTP,&sa_tstp,NULL);

	sa_chld.sa_sigaction = sigchld_handler;
	sigfillset(&sa_chld.sa_mask);
	sa_chld.sa_flags = SA_SIGINFO | SA_RESTART;
	sigaction(SIGCHLD,&sa_chld,NULL);



    if(argc == 2){
        input = fopen(argv[1], "r");
	if(!input){
	    perror("Cannot open specified file");
	    return 1;
	}
	is_script = 1;
    } else if(argc > 2){
		fprintf(stderr, "Format: %s script_file\n", argv[0]);
        return 1;
    }

    if(!is_script){
        printf("Starting IC shell\n");
    }

    while (1) {
		if(!is_script){
			prompt();
		}
		if(fgets(buffer, MAX_CMD_BUFFER, input)==NULL){
			break;
		}
		newline(buffer);
		if(strlen(buffer)==0){
			continue;
		}

		if(strcmp(buffer, "jobs")==0){
			print_jobs();
			continue;
		}

		//Double-bang:repeats last command
		if(strcmp(buffer, "!!") == 0) {
			if(strlen(last_cmd)==0){
				continue;
			}
			if(!is_script){
				printf("%s\n", last_cmd);
			}
			strncpy(buffer,last_cmd, MAX_CMD_BUFFER);
		}else{
			strncpy(last_cmd,buffer,MAX_CMD_BUFFER);
		}

		char *cmd = strtok(buffer, " ");
		if(!cmd){
			continue;
		} 

		char *args[MAX_CMD_BUFFER];
		int i = 0;
		char *in = NULL;
		char *out = NULL;
		int is_bg = 0;

		while(cmd){
			if(strcmp(cmd, "<")==0){
				cmd = strtok(NULL, " ");
				in = cmd;
			}else if(strcmp(cmd,">")==0){
				cmd = strtok(NULL, " ");
				out = cmd;
			}else if(strcmp(cmd, "&")==0){
				is_bg=1;
			}else{
				args[i++] = cmd;
			}
			cmd = strtok(NULL," ");
		}
		args[i]=NULL;
		if(args[0]==NULL){
			continue;
		}

		//echo: prints given text
		if(strcmp(args[0], "echo") == 0){
			if(args[1] && strcmp(args[1], "$?")==0){
				printf("%d\n",last_exit_status);
			}else {
				for(int j=1;args[j];j++){
					printf("%s ", args[j]);
				}
				printf("\n");
			}
			last_exit_status=0;
			continue;
		}

		//exit
		if(strcmp(args[0], "exit") ==0){
			int code = 0;
			if(args[1]){
				code = atoi(args[1]) & 0xFF;	
			}
			if(!is_script){
				printf("bye\n");
			}
			if(is_script){
				fclose(input);
			}
			free_jobs();
			return code;
		}

		if(strcmp(args[0], "fg")==0 && args[1] && args[1][0] == '%'){
			int jid = atoi(args[1]+1);
			Job *job = get(jid);
			if(job){
				foreground_pid = job->pid;
				job->status = RUNNING;
				kill(job->pid, SIGCONT);
				printf("%s\n", job->command);
				int status;
				waitpid(job->pid, &status, WUNTRACED);
				if(WIFEXITED(status)||WIFSIGNALED(status)){
					remove_job(job->pid);
				}else if (WIFSTOPPED(status)){
					job->status = STOPPED;
				}
				foreground_pid = -1;
			}
			continue;	
		}

		if(strcmp(args[0],"bg")==0 && args[1] && args[1][0] == '%'){
			int jid = atoi(args[1]+1);
			Job *job = get(jid);
			if(job && job->status == STOPPED){
				job->status = RUNNING;
				kill(job->pid, SIGCONT);
				printf("[%d]+ %s &\n",job->job_id, job->command);
			}
			continue;
		}

		pid_t pid = fork();
		if(pid<0){
			perror("fork failed");
			continue;
		}else if (pid==0){
			if(in){
				int in_file = open(in,O_RDONLY);
				if(in_file<0){
					perror("Input file error");
					exit(1);
				}
				dup2(in_file,0);
				close(in_file);
			}
			if(out){
				int out_file = open(out,O_WRONLY|O_CREAT|O_TRUNC,0666);
				if(out_file<0){
					perror("Output file error");
					exit(1);
				}
				dup2(out_file,1); 
				close(out_file);
			}
			execvp(args[0],args);
			perror("bad command");
			exit(1);
		}else{
			if(is_bg){
				add_job(pid, last_cmd, RUNNING);
				printf("[%d] %d\n",  next_jid-1, pid);
			}else{
				foreground_pid = pid;
				int status;
				waitpid(pid, &status, WUNTRACED);
				if(WIFEXITED(status)){
					last_exit_status = WEXITSTATUS(status);
				}else if(WIFSTOPPED(status)){
					int jid = add_job(pid,last_cmd, STOPPED);
					printf("[%d]+  Stopped\t%s\n",jid,last_cmd);
				}
				foreground_pid = -1;
			}
		}
    }
    if(is_script){
        fclose(input);
    }
    return 0;
}
