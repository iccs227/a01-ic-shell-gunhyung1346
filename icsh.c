/* ICCS227: Project 1: icsh
 * Name: GunHyung Kim
 * StudentID: 6480233
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CMD_BUFFER 255

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
	    if(args){
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

	printf("bad command\n");
    }

    if(is_script){
        fclose(input);
    }

    return 0;
}
