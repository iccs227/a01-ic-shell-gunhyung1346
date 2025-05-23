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

#define MAX_CMD_BUFFER 255
pid_t foreground_pid = -1;
int last_exit_status = 0;

void sigint_handler(int sig, siginfo_t *si, void *notused){
	if(foreground_pid>0){
		kill(foreground_pid,SIGINT);
	}
	printf("\n");
}

void sigtstp_handler(int sig, siginfo_t *si, void *notused){
	if(foreground_pid>0){
		kill(foreground_pid,SIGTSTP);
	}
	printf("\n");
}

void prompt(){
    printf("icsh $ ");
    fflush(stdout);
}

void newline(char *str){
    str[strcspn(str, "\n")] = 0;
}

int main(int argc, char *argv[]) {
    char buffer[MAX_CMD_BUFFER];
    char last_cmd[MAX_CMD_BUFFER] = "";
    FILE *input = stdin;
    int is_script = 0;

	struct sigaction sa_int, sa_tstp;
	sa_int.sa_sigaction = sigint_handler;
	sigfillset(&sa_int.sa_mask);
	sa_int.sa_flags = SA_SIGINFO;
	sigaction(SIGINT,&sa_int,NULL);

	sa_tstp.sa_sigaction = sigtstp_handler;
	sigfillset(&sa_tstp.sa_mask);
	sa_tstp.sa_flags = SA_SIGINFO;
	sigaction(SIGTSTP,&sa_tstp,NULL);

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

		//Double-bang:repeats last command
		if(strcmp(buffer, "!!") == 0) {
			if(strlen(last_cmd)==0){
				continue;
			}
			if(is_script){
			}else{
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
		
		//echo: prints given text
		if(strcmp(cmd, "echo") == 0){
			char *args = strtok(NULL, "");
			if(args && strcmp(args, "$?")==0){
				printf("%d\n",last_exit_status);
			}else if(args){
				printf("%s\n", args);
			}
			continue;
		}

		//exit
		if(strcmp(cmd, "exit") ==0){
			char *arg = strtok(NULL, " ");
			int code = 0;
			if(arg){
				code = atoi(arg) & 0xFF;	
			}
			if(!is_script){
			printf("bye\n");
			}
			if(is_script){
				fclose(input);
			}
			return code;
		}

		pid_t pid = fork();
		if(pid<0){
			perror("fork failed");
			continue;
		}else if (pid==0){
			char *args[64];
			int i = 0;
			args[i++] = cmd;
			char *token;
			while((token = strtok(NULL, " "))!=NULL){
				args[i++] = token;
			}
			args[i] = NULL;
			execvp(cmd,args);
			perror("Command not found");
			exit(1);
		}else{
			foreground_pid = pid;
			int status;
			waitpid(pid, &status, WUNTRACED);
			foreground_pid = 0;
		}

    }
    if(is_script){
        fclose(input);
    }

    return 0;
}
