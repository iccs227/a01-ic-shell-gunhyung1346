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

int main() {
    char buffer[MAX_CMD_BUFFER];
    char last_cmd[MAX_CMD_BUFFER] = "";

    printf("Starting IC shell\n");
    while (1) {
        prompt();
	if(fgets(buffer, MAX_CMD_BUFFER, stdin)==NULL){
	    break;
	}

	buffer[strcspn(buffer, "\n")] = 0;

	//Double-bang:repeats last command
	if(strcmp(buffer, "!!") == 0) {
	    if(strlen(last_cmd)==0){
	        continue;
	    }
	    printf("%s\n", last_cmd);
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
	    printf("bye\n");
	    exit(code);
	}

	printf("bad command\n");
    }
    return 0;
}
