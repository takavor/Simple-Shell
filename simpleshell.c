#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>

/**
 * Sevag Baghdassarian - 260980928 - McGill University - COMP 310 Assignment 1
 * 
 * 8/10/2021
 * 
 * */

// global int variable denoting number of background jobs
int numBackgroundJobs = 0;

// struct defining a job, to be used to store commands that are being run in background mode in an array
struct job {
    pid_t pid;
    char *commandName;
    int isInBackground;
};

// initial capacity of the array of background jobs
int jobsCapacity = 50;
struct job *backgroundJobs;

int getcmd(char *prompt, char *args[], int *background) {
    
    int length, i = 0;
    char *line = NULL;
    size_t linecap = 0;

    printf("%s", prompt);
    length = getline(&line, &linecap, stdin);

    char *token = malloc(linecap * sizeof(char));

    if (length <= 0) {
        exit(-1);
    } else if(length == 1) {
        return 0;
    }

    while ((token = strsep(&line, " \t\n")) != NULL) {

        for (int j = 0; j < strlen(token); j++) {
            if (token[j] <= 32) {
                token[j] = '\0';
            }
        }

        if (strlen(token) > 0) {
            args[i++] = token;

            // Check if background is specified and set the value of the background variable
            if(strcmp(token, "&") == 0) {
                *background = 1;
            } else {
                *background = 0;
            }
            
        }
    }
    
    // handle case where input consists of only &
    if(length == 2 && *args[i-1] == '&') {
        *background = 0;
    }

    // terminate the args array with null variable so that execvp works as expected
    args[i] = '\0';
    free(token);
    return i; // i = number of arguments
}

// signal handler function for when ctrl+c and ctrl+z are pressed in the mini shell
static void parentSigHandler(int sig) {
    if(sig == SIGINT) {
        // do nothing to prevent ctrl+c from stopping the mini shell
        printf("Terminated process.\n");
    } 
    else if(sig == SIGTSTP) {
        // do nothing to prevent ctrl+z from suspending the mini shell
        printf("Suspended process.\n");
    }
}

// Implementing the functionality of fg command
// args: parsed command
// cnt: number of arguments
// jobs: array containing background jobs
// jobsStored: size of jobs array
// returns 0 if process is brought to foreground, -1 otherwise
int executeFg(char **args, int cnt, struct job *jobs, int jobsStored) {
    
    // if there is only 1 argument or none, no job ID specified
    if(cnt < 2) {
        printf("error: expected a job number\n");
        return -1;
    }

    // get specified job number, convert the string to an int
    int jobNumber = atoi(args[1]);
    struct job thisJob;

    // get process id of job at index jobNumber -1
    if(jobNumber >= 1 && jobNumber <= jobsStored) {
        thisJob = jobs[jobNumber - 1];
    } else {
        printf("error: entry not found\n");
        return -1;
    }

    pid_t pid = thisJob.pid; 

    // send a signal to continue the specified process
    if(kill(pid, SIGCONT) < 0) {
        printf("error: could not find job with pid %d\n", pid);
        return -1;
    }

    // update background field of job coming to foreground
    thisJob.isInBackground = 0;

    // update background field of all other jobs
    for(int i = 0; i < jobsStored; i++) {
        if(i != jobNumber - 1) {
            jobs[i].isInBackground = 1;
        }
    }
    
    // the tcsetpgrp function sets the pid specified in 2nd argument as the foreground process

    // process will be set to the foreground using tcsetpgrp function
    // when tcsetpgrp is called by a process in the background, a SIGTTOU signal is sent
    // this signal alerts for when a background process tries to write to the terminal
    // so, ignore this signal 
    signal(SIGTTOU, SIG_IGN);
    // set the process executing fg as the foreground process
    tcsetpgrp(0, getpid());
    // the process is now in the foreground, do not ignore the signal for when it tries to write to terminal
    signal(SIGTTOU, SIG_DFL);

    return 0;

}

// helper function to display the jobs when the jobs command is entered
// the function will print out the job number (index of the job in jobs array),
// the process id corresponding to the job, and the command name
int displayJobs(struct job *jobs, int jobsStored, int wantsOutputRedir, char *fileName) {

    // for each job in the jobs array, check if the job is in the background
    // if it is, display it    
    if(jobsStored != 0) {

        FILE *filePtr;
        
        // line variable will contain the line to write to the file/print to console
        char *line;
        line = malloc(4096 * sizeof(char));

        // set up titles for the columns in a formatted way
        sprintf(line, "%6s\t\t%10s\t\t%7s", "Job ID", "Process ID", "Command\n");

        if(wantsOutputRedir == 0) {
            // print the line to terminal
            printf("%s", line);
        } else {
            // open the file for writing
            filePtr = fopen(fileName, "w");
            // write line to the file
            fputs(line, filePtr);
        }

        // loop through the jobs array to get each job
        for(int i = 0; i < jobsStored; i++) {
            
            struct job current = jobs[i];  
            if(current.isInBackground == 1) {
                
                // set line to be the job ID, process ID, and command name
                sprintf(line, "%6d\t\t%10d\t\t%7s\n", i+1, current.pid, current.commandName);

                if(wantsOutputRedir == 0) {
                    // print jobs info to terminal
                    printf("%s", line);
                } else {
                    // write job info to file
                    fputs(line, filePtr);
                }
            }

        }

        // close the file
        if(wantsOutputRedir == 1) {
            fclose(filePtr);
        }
        free(line);
        return 1;
    }
    
    printf("no jobs to display\n");
    return -1;
}

// Function to handle built-in commands execution
// args: parsed command
// cnt: number of arguments
// jobs: array containing background jobs
// jobsStored: jobs array size (total number of jobs stored)
int builtInCmdHandler(char **args, int cnt, struct job *jobs, int jobsStored) {
    
    // I will make an array containing all the built-in commands to be implemented
    // Then, I will verify the command that is requested in **args
    // If it matches one of the commands in the array, the code will be executed based on
    // the desired command
    // cd, pwd, exit, fg, jobs
    // function will return the index of the corresponding builtin command if it is requested, -1 if none of them are requested
    // and it will automatically execute the commands except for pwd and jobs
    // in the case of pwd and jobs, which may require output redirection, it will just return their indices
    int cmds = 5;
    int mode = -1; // dummy value; mode indicates the built-in command to execute
    
    // set up array containing built-in commands
    char *builtInCmds[cmds];
    builtInCmds[0] = "cd";
    builtInCmds[1] = "pwd";
    builtInCmds[2] = "exit";
    builtInCmds[3] = "fg";
    builtInCmds[4] = "jobs";

    // check if command argument matches any of these
    for(int i = 0; i < cmds; i++) {
        if(strcmp(args[0], builtInCmds[i]) == 0) {
            mode = i;
            break;
        }
    }

    // mode will correspond to the command which is requested. if mode == -1, no builtin commands were requested

    // determine which built-in command is requested
    switch(mode) {
        
        case 0: // cd
            chdir(args[1]); // 2nd argument is the directory to go to
            return 0;

        case 1: // pwd
            return 1;

        case 2: // exit
            printf("See you later boss!\n");
            exit(0);
            return 2;

        case 3: // fg
            executeFg(args, cnt, jobs, jobsStored);
            return 3;

        case 4: // jobs
            return 4;
            
    }
    // return -1 if no built in commands were requested
    return -1;
}


// Function to check whether output redirection is requested in the command
// Output redirection is requested with the '>' character
// **args: inputted command
// **fileName: name of the file to which to redirect output
// Returns 1 if output redirection is requested, 0 if not specified,
// -1 if there is an error (no file provided)
int wantsOutputRedirection(char **args, char **fileName) {

    // check if string contains '>'
    // args is the array of the arguments
    // if '>' is there, it will be the first character of the argument at index i
    for(int i = 0; args[i] != NULL; i++) {
        if(args[i][0] == '>') {

            // output redirection is requested
            // get the name of the file
            if(args[i + 1] == NULL) {
                // if there is no filename after the >, return -1
                return -1;
            } else {
                *fileName = args[i + 1];
            }

            // get rid of everything after the > in args
            for(int j = i - 1; args[j] != NULL; j++) {
                
                // i is the index where we found the >
                // I am assuming that there is no important information after the fileName
                // this logic will ignore anything that comes after the fileName, and whatever comes
                // before the > will be regarded as the command to execute, the output of which
                // will be redirected to the file
                args[j + 1] = args[j + 3];

            }

            // return 1 to indicate that the command requires output redirection
            return 1;
        }
    }

    // if reached here, no > was found: no output redirection requested
    return 0;
}


// handler to catch a signal for when a child process running in the background has terminated
static void backgroundChildTerminated(int sig) {
    numBackgroundJobs--;
}

// function to determine whether the command requires a pipe (look for | character)
// **args: inputted command
// **nextArg: command that will read from the pipe
// returns 1 if command requires piping, 0 otherwise
int wantsPipe(char **args, char **nextCmd) {

    for(int i = 0; args[i] != NULL; i++) {

        // check if current string is equal to |
        if(args[i][0] == '|') {
            
            // get the command that will read from the pipe
            int k = 0;
            for(int j = i+1; args[j] != NULL; j++) {
                nextCmd[k++] = args[j];
            }
            // null-terminate the command so that it may be executable with execvp
            nextCmd[k] = '\0';

            // nullify the rest of args
            for(int m = i; args[i] != NULL; m++) {
                args[m] = NULL;
            }

            return 1;

        }

    }

    return 0;
}

// helper function to pipe the commands
int pipeCommands(char **firstCmd, char **secondCmd) {

   
    // create a pipe
    int fd[2];
    if(pipe(fd) < 0) {
        printf("error: could not pipe\n");
        exit(EXIT_FAILURE);
    }

    // fork to execute first command
    int pid = fork();

    if(pid < 0) {
        printf("error: could not fork\n");
        exit(EXIT_FAILURE);
    }

    if(pid == 0) {
        // child process here

        // send output to pipe instead of terminal
        dup2(fd[1], STDOUT_FILENO);

        // execute the command
        if(execvp(firstCmd[0], firstCmd) < 0) {
            printf("error: could not execute command\n");
            exit(EXIT_FAILURE);
        }
    }

    // child process won't reach here since it will do execution

    // get output of the child execution and make it the input of the next one
    dup2(fd[0], STDIN_FILENO);

    // close write end of pipe since it won't be used
    close(fd[1]);

    // execute the next command
    if(execvp(secondCmd[0], secondCmd) < 0) {
        printf("error: could not execute command\n");
        exit(EXIT_FAILURE);
    }               

    return 1;        

}

int main() {
    
    char *args[20], *fileName;
    int bg, outputRedirRequested;
    
    if(signal(SIGINT, parentSigHandler) == SIG_ERR) {
        printf("error: could not bind SIGINT signal handler\n");
        exit(1);
    }

    if(signal(SIGTSTP, parentSigHandler) == SIG_ERR) {
        printf("error: could not bind SIGTSTP signal handler\n");
        exit(1);
    }

    // allocate memory for an array of "job" objects
    // if the command is requested in background mode, I will add the job to the array
    // index of the job (+1) will correspond to the job number, and inside that index will be stored the 
    // job object holding the pid and the name of the command
    // initially, I will support holding 50 jobs inside the array. 
    // If the number of jobs surpasses, I will reallocate memory for the array and double the capacity
    int jobsStored = 0; // counter for how many jobs have been stored in the array
    //int jobsCapacity = 50;
    int activeJobs = 0; // counter for how many background jobs are currently active
    struct job *backgroundJobs = malloc(sizeof(struct job) * jobsCapacity);

    while(1) {

        bg = 0;

        int status;
        int cnt = getcmd("\n>>  ", args, &bg);
        
        // If input is empty, ignore
        if(cnt == 0) {
            continue;
        }

        // before forking, verify if output redirection is requested
        outputRedirRequested = wantsOutputRedirection(args, &fileName);

        if(outputRedirRequested == -1) {
            // if -1 is returned, filename could not be retrieved
            printf("error: filename or command not specified\n");
            continue;
        }
        
        // get the requested builtin command (if any)
        // mode == 1 => pwd
        // mode == 4 => jobs
        int mode = builtInCmdHandler(args, cnt, backgroundJobs, jobsStored);

        // if builtin command wasn't requested (return -1), do not skip loop
        // skip loop for commands that are already executed in builtInCmdHandler
        // (everything except for pwd and jobs)
        if(mode != 1 && mode != 4 && mode != -1) {
            continue;
        }

        char **nextCmd;
        nextCmd = malloc(4096 * sizeof(char));

        int shouldPipe = wantsPipe(args, nextCmd);

        char *cwd;

        // handle pwd and jobs seperately for if they request output redirection
        if(mode == 1) {
            
            // allocate sufficient memory for the path to display
            // PATH_MAX is defined to be 4096 bytes in <limits.h> but it is not being recognized on
            // every system, so I have hardcoded the value 
            cwd = malloc(4096 * sizeof(char));

            if(getcwd(cwd, 4096 * sizeof(char)) == NULL) {
                printf("Error: could not get current directory\n");
                free(cwd);
                continue;
            }

            if(outputRedirRequested == 1) {

                // file pointer to requested file 
                FILE *filePtr;
                // open the file for writing
                filePtr = fopen(fileName, "w");

                if(filePtr == NULL) {
                    printf("error: could not find file\n");
                    free(cwd);
                    continue;
                }
                
                fputs(cwd, filePtr);
                fclose(filePtr);

            } else {
                // output redir not requested, print the pwd
                printf("%s\n", cwd);
            }

            free(cwd);
            // go to next iteration of while loop
             continue;

        } else if(mode == 4) {

            displayJobs(backgroundJobs, jobsStored, outputRedirRequested, fileName);
            // go to next iteration of while loop
            continue;
        }

        // fork to create a child process in which to execute command
        pid_t childpid = fork();

        if(childpid < 0) {

            printf("error: could not fork\n");
            exit(EXIT_FAILURE);

        } else if(childpid == 0) {
            
            // child process here

            // if piping requested, pipe
            if(shouldPipe == 1) {
                
                pipeCommands(args, nextCmd);

            }
            
            // if output redirection requested, redirect
            // the function freopen opens the 1st argument file and closes the 3rd argument stream
            // for the function specified in the 2nd argument (read or write)
            // in our case, 1st argument will be the filename in the command (open it for writing)
            // and the stream will be stdout, which would normally be where the output would be displayed
            // 2nd argument will be writing mode
            if(outputRedirRequested == 1) {
                freopen(fileName, "w+", stdout);
            }

            // replace & with NULL to execute command
            if(bg == 1) {
                args[cnt -1] = '\0';
            }

            // execute the command and get return value
            int execStatus = execvp(args[0], args);

            // if code reaches here, execution was not successful
            if(execStatus < 0) {
                printf("could not find command %s\n", args[0]);
                exit(EXIT_FAILURE);
            } else {
                if(bg == 1) {
                    // if execution was successful, increment numBackgroundJobs variable
                    numBackgroundJobs++;
                }
            }
                        
        } else {
            
            // parent process here

            if(bg == 0) {
                // background mode not requested
                waitpid(childpid, &status, WUNTRACED);
                int exit_status = WEXITSTATUS(status);
                //printf("\nExit status of child: %d\n", exit_status);
            } else {
                // background mode requested

                // catch a signal for when child finished
                signal(SIGCHLD, backgroundChildTerminated);

                // check if the array is over capacity
                if(jobsStored >= jobsCapacity) {
                    jobsCapacity *= 2;
                    // reallocate memory for the array
                    backgroundJobs = realloc(backgroundJobs, sizeof(struct job) * jobsCapacity);
                }
                
                // create a job struct with the command name and child pid
                struct job thisJob;
                thisJob.pid = childpid;
                thisJob.commandName = args[0];
                thisJob.isInBackground = 1;

                // set the job inside the array
                backgroundJobs[jobsStored] = thisJob;

                jobsStored++;

            }

        }
        
        free(nextCmd);
        
    }
    free(backgroundJobs);
}   