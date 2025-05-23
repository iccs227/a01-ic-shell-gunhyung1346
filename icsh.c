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

#define MAX_CMD_BUFFER 255
pid_t foreground_pid = -1;
int last_exit_status = 0;

void sigint_handler(int sig, siginfo_t *si, void *notused){
	if(foreground_pid>0){
		kill(foreground_pid,SIGINT);
		printf("\n");
	}
}

void sigtstp_handler(int sig, siginfo_t *si, void *notused){
	if(foreground_pid>0){
		kill(foreground_pid,SIGTSTP);
		printf("\n");
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
		char *args[MAX_CMD_BUFFER];
		int i = 0;
		char *in = NULL;
		char *out = NULL;

		while(cmd){
			if(strcmp(cmd, "<")==0){
				cmd = strtok(NULL, " ");
				in = cmd;
			}else if(strcmp(cmd,">")==0){
				cmd = strtok(NULL, " ");
				out = cmd;
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
			return code;
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
			perror("Command not found");
			exit(1);
		}else{
			foreground_pid = pid;
			int status;
			waitpid(pid, &status, WUNTRACED);
			if(WIFEXITED(status)){
				last_exit_status = WEXITSTATUS(status);
			}
			foreground_pid = 0;
		}

    }
    if(is_script){
        fclose(input);
    }

    return 0;
}
