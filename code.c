#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>

#define MAX_LENGTH 80

int set_token(char *args[MAX_LENGTH/2 + 1], char his[80]) {
    
    char *token;
    token = strtok(his, " ");

    int size = 0;
    while (token != NULL) {
        args[size++] = token;
        //printf("%s\n",token);
        token = strtok(NULL, " ");
    }
    
    return size;
}


int main(void) {

    char *args[MAX_LENGTH/2 + 1]; // main commands
    int should_run = 1; // check if continue to run
    char history[MAX_LENGTH]; // store history string
    int savehis = 0; // check if history is saved

    while(should_run) {
        
        printf("ssh> ");
        fflush(stdout);
        //fflush(stdin);

        for(int i = 0; i < MAX_LENGTH/2 + 1; i++) args[i]='\0'; // clearn commands

        char command[80], dummy[80];
        fgets(command, 80, stdin); // get command input from user
        
        strtok(command,"\n");
        strcpy(dummy,command);
        
        int nwait = 0;
        int size = set_token(args, dummy);
        
        if (size == 1 && strcmp(args[0], "!!") == 0) {
            if (savehis) {
                // use history
                for(int j = 0; j < MAX_LENGTH/2 + 1; j++) args[j]='\0';
                strcpy(dummy, history);
                size = set_token(args, dummy);
            }
            else {
                // no history available
                printf("There is no history yet\n");
                continue;
            }

        }
        else {
            // save history and check flag
            savehis = 1;
            strcpy(history, command);
        }


        if (strcmp(args[0], "exit") == 0) { // exit the shell
            should_run = 0;
            break;
        }

        if (strcmp(args[size - 1], "&") == 0) { // check if wait for child process
            nwait = 1;
            args[size - 1] = NULL;
            size--;
        }

        int isPipe = 0;
        for (int k = 0; k < size; k++) 
            if (strcmp(args[k],"|") == 0) { // check of pipe is availabel
                isPipe = 1; 
                break;
            }

        if (isPipe) { // pipe case

            char *agrs1[MAX_LENGTH/2 + 1]; for(int k = 0; k < MAX_LENGTH/2 + 1; k++) agrs1[k]='\0';
            char *agrs2[MAX_LENGTH/2 + 1]; for(int k = 0; k < MAX_LENGTH/2 + 1; k++) agrs2[k]='\0';

            int idx = 0, change = 0;
            for (int k = 0; k < size; k++) { // split commands
                if (strcmp(args[k], "|") == 0) { change = 1; idx = 0;  }
                else {
                    if (change == 0) agrs1[idx++] = args[k];
                    else agrs2[idx++] = args[k];
                }
            }

            int fd[2];
            pid_t pid1, pid2;
            pipe(fd);

            // create the first child and run the first command
            pid1 = fork();
            
            if (pid1==0) {
                // set the process output to the input of the pipe
                close(1);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
                
                execvp(agrs1[0], agrs1);
                return -1;
            }

            // create the second child and run the second command
            pid2 = fork();
           
            if (pid2==0) {
                // set the process input to the output of the pipe
                close(0);
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                close(fd[1]);
                
                execvp(agrs2[0], agrs2);
                return -1;
            }

            close(fd[0]);
            close(fd[1]);
            
            if (nwait == 0) { // Wait for the children to finish
                waitpid(pid1,NULL,0);
                waitpid(pid2,NULL,0);
            }
            else { // not wait and sleep about 1 second
                sleep(1);
            }

        }
        else { // solve no pipe case
            pid_t id = fork();

            if (id == 0) {

                int change = 0, in = 0, out = 0;

                for(int k=0;args[k]!='\0';k++) {

                    if(strcmp(args[k],"<")==0) { in=1; change = 1; }

                    if(strcmp(args[k],">")==0) { out=1; change = 1; }

                    if (change) args[k]=args[k + 1];
                }

                if (in) { // if exit "<"
                    int fd0 = open(args[1], O_RDONLY, 0);
                    dup2(fd0, STDIN_FILENO);
                    close(fd0);
                }
                
                if (out) { // if exit ">"
                    int fd1 = creat(args[1] , 0644) ;
                    dup2(fd1, STDOUT_FILENO);
                    close(fd1);
                }

                execvp(args[0],args);
                return -1;
            }
            
            if (nwait) { // not wait and sleep about 1 second
                sleep(1);
            }
            else { // Wait for the children to finish 
                while(wait(NULL) != id);
            }
            
        }
    }



    return 0;
}

