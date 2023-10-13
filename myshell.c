/* author: qhc */

#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#define MODE 0666

typedef struct cmd {
    int infd;
    int outfd;
    char *argv[20];
} CMD;

CMD cmd0;
CMD cmd1;

void parseInput();
int parseArgv(char *instr, char *argv[]);
void run();

char input[1024] = {0};
int status;         // for process waiting
int fd[2];          // for pipe
int pid;

int pipeFlag;
int redirectInFlag;
int redirectOutFlag;
int redirectOutAddFlag;
int cdFlag;


int main() {
    while(1) {
        printf("qhcshell$ ");
        fgets(input, 1024, stdin);
	parseInput();
	run();
    }
    return 0;
}

void parseInput() {
    if (input[strlen(input) - 1] == '\n') {
        input[strlen(input) - 1] = '\0';
    }
    for (int i=0; i<strlen(input); i++) {
        if (input[i] == '\t') input[i] == ' ';
    }

    pipeFlag = 0;
    redirectInFlag = 0;
    redirectOutFlag = 0;
    redirectOutAddFlag = 0;
    cdFlag = 0;

    if (strstr(input, "|")) {
    	pipeFlag = 1;
    }
    else if (strstr(input, ">>")) {
    	redirectOutAddFlag = 1;
    }
    else if (strstr(input, ">")) {
        redirectOutFlag = 1;
    }
    else if (strstr(input, "<")) {
        redirectInFlag = 1;
    }
    else if (strncmp(input, "cd", 2) == 0) {
        cdFlag = 1;
    }
}

int parseArgv(char *instr, char *argv[]) {
    argv[0] = strtok(instr, " ");
    int i = 0;
    while (argv[i]) {
        argv[i+1] = strtok(NULL, " ");
	i++;
    }
    if (argv[0] && strcmp(argv[0], "exit") == 0 && !argv[1]) {
	return 1;	// is exit
    }
    else return 0;
}

void run() {

    if (redirectInFlag) {
        char *targetStr = strtok(input, "<");
        char *sourceStr = strtok(NULL, "<");
        if (parseArgv(targetStr, cmd0.argv) == 1) {
            exit(0);
        }
        parseArgv(sourceStr, cmd1.argv);
        if (cmd1.argv[1]) {
            printf("Redirect: too many source file\n");
            return;
        }
        if((pid = fork()) < 0) {
            perror("Fork error");
            return;
        }
        else if (pid > 0) {
            redirectInFlag = 0;
            memset(input, 0, 1024);
            waitpid(pid, &status, 0);
        }
        else {
            if ((cmd0.infd = open(cmd1.argv[0], O_RDONLY)) < 0) {
                perror("Open failed");
                exit(-1);
            }
            dup2(cmd0.infd, STDIN_FILENO);
	    close(cmd0.infd);
            execvp(cmd0.argv[0], cmd0.argv);
            perror("Execvp error");
            exit(-1);
        }
    }

    else if (redirectOutFlag || redirectOutAddFlag) {
        char *sourceStr = strtok(input, ">");
	char *targetStr = strtok(NULL, ">");
	if (parseArgv(sourceStr, cmd0.argv) == 1) {
	    exit(0);
	}
	parseArgv(targetStr, cmd1.argv);
	if (cmd1.argv[1]) {
	    printf("Redirect: too many target file\n");
	    return;
	}
	if((pid = fork()) < 0) {
	    perror("Fork error");
	    return;
	}
	else if (pid > 0) {
	    redirectOutFlag = 0;
            redirectOutAddFlag = 0;	    
	    memset(input, 0, 1024);
	    waitpid(pid, &status, 0);
	}
	else {
            int oflag = redirectOutFlag == 1 ? O_WRONLY|O_CREAT|O_TRUNC : O_WRONLY|O_CREAT|O_APPEND; 
	    if ((cmd0.outfd = open(cmd1.argv[0], oflag, MODE)) < 0) {
                perror("Open failed");
                exit(-1);     
            }
	    dup2(cmd0.outfd, STDOUT_FILENO);
	    close(cmd0.outfd);
	    execvp(cmd0.argv[0], cmd0.argv);
	    perror("Execvp error");
	    exit(-1);
	}
    }

    else if (cdFlag) {
        parseArgv(input, cmd0.argv);
	char *path = cmd0.argv[1];
	if (path && cmd0.argv[2]) {
	    printf("Too many arguments\n");
	    return;
	}
	else {
	    chdir(path);
	}
    }

    else if (pipeFlag) {
        char *sourceStr = strtok(input, "|");
	char *targetStr = strtok(NULL, "|");
	parseArgv(sourceStr, cmd0.argv);
	parseArgv(targetStr, cmd1.argv);
        if ((pid = fork()) < 0) {
	    perror("Fork error");
	    return;
	}
	else if (pid > 0) {
	    pipeFlag = 0;
	    memset(input, 0, 1024);
	    waitpid(pid, &status, 0);
	}
	else {
	    if (pipe(fd) < 0) {
	        perror("Pipe create unsuccessful");
		exit(-1);
	    }
	    if ((pid = fork()) < 0) {
	        perror("Fork error");
		exit(-1);
	    }
	    else if (pid > 0) {
	        dup2(fd[0], STDIN_FILENO);
		close(fd[0]);
		close(fd[1]);
		execvp(cmd1.argv[0], cmd1.argv);
                perror("Execvp error");
                exit(-1);
	    }
	    else {
	        dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
                execvp(cmd0.argv[0], cmd0.argv);
                perror("Execvp error");
                exit(-1);
	    }
	}

    }

    else {          // a normal command	

	if (parseArgv(input, cmd0.argv) == 1) { // is exit
	    exit(0);
	}
        if (!cmd0.argv) return; // empty input
	
	if ((pid = fork()) < 0) {
            perror("Fork error");
	    return;
        }
        else if (pid > 0) {
            waitpid(pid, &status, 0);
	    memset(input, 0, 1024);
        }
        else {
            execvp(cmd0.argv[0], cmd0.argv);
	    perror("Execvp error");
            exit(-1);
	}
	
    }

}
