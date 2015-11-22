/*
Description: OSU_CS344_Assignment_3, Smallsh
Language:C
Author: Jiongcheng Luo
Date:	11/20/15
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_CMD_LENGTH 2048
#define MAX_CMD_COUNT 512
#define MAX_PATH_LENGTH 1024

int exec_fg_cmd(int,char**);
int exec_bg_cmd(int,char**);
int check_bg_exec(int,char**);
void get_fg_status(int);
void IntHandler(int);

/*###############################################################
								MAIN
################################################################*/
int main(void){
	char line[MAX_CMD_LENGTH];		//input
	char *argv[MAX_CMD_LENGTH];     //command
	int argc;						//cmd count
	size_t ipt_len;
	char *token;    
	              	//split command into separate strings
	char cmdpath[32];
	char *PATH = getenv("PATH");

	int bg_cmd = 0;
	int fg_status = 0, bg_status = 0; 	//defualt process status as 0
	pid_t bg_pid, c_pid;

	/*########## ctrl c handler ##########*/
	struct sigaction act;
	act.sa_handler = IntHandler;
	sigaction(SIGINT, &act, NULL);

	while(1){
	/*########## Promte&parse command ##########*/
		printf(":");
		//########## catch any background process completed ##########
		c_pid = waitpid(bg_pid, &bg_status, WNOHANG | WUNTRACED);		//catch background process finished
		if(c_pid == bg_pid){	
			printf("background pid %d is done: ", c_pid);
			fflush(NULL);
			if (WIFEXITED(bg_status)){			//returns true if the child terminated normally
                printf("Exit value %d\n", WEXITSTATUS(bg_status));
                fflush(NULL);
            }else if (WIFSIGNALED(bg_status)) {
                printf("Terminated by signal %d\n", WTERMSIG(bg_status));
                fflush(NULL);
            }
		}
		fgets(line, MAX_CMD_LENGTH, stdin);
		if(line[0]!='\n' && line[0]!='#'){	//catch newline or comment input->no action
			ipt_len = strlen(line);
			if(line[ipt_len-1] == '\n')
				line[ipt_len-1] = '\0';		//replace \n to space
			token = strtok(line," ");		//check space
			int i=0;
    		while(token!=NULL){				//parse input string to **argv from space
        		argv[i]=token;      
        		token = strtok(NULL," ");
        		i++;
   		 	}
    		argv[i]=NULL;                     //set last value to NULL for execvp
    		argc=i;                           //get arg count

	/*########## Read build in command ##########*/
	  		if(argc <= 2 && strcmp(argv[0],"cd")==0){	//check for 'cd' cmd
				if(argc==1){			//cd to home path
 					chdir(getenv("HOME"));		
 				}else if(argc==2){
  					chdir(argv[1]);		//cd to cmd path
  				}else{
  					perror("cd");
  					exit(1);
  				}
			}else if(argc == 1 && strcmp(argv[0],"status")==0){		//check signal status from foreground
				get_fg_status(fg_status);
			}else if(argc == 1 && strcmp(argv[0],"exit")==0){
				exit(0);	//program terminated
			}else{
	/*########## else Read system command ##########*/
				if(check_bg_exec(argc, argv)==1){	
					pid_t tmp_pid;
					bg_pid = fork();
					if(bg_pid == -1){	//error in fork();
						perror("'fork()'");
						exit(1);
					/*############## check background cmd ###############*/
					}else if(bg_pid == 0){	//child process successed
					//if(argc > 1 && strcmp(argv[argc-1],"&")==0){	
						tmp_pid = getpid();
						printf("background pid is %d\n", tmp_pid);
						fflush(NULL);
						printf("\n");
						struct sigaction act;
						act.sa_handler = SIG_IGN;
						sigaction(SIGINT, &act, NULL);
						argv[argc-1] = NULL;	//change background command to legit cmd
						execvp(argv[0],argv);
    					fprintf(stderr, "%s: No such file or directory\n", argv[0]);
    					fflush(NULL);
    					exit(1);
					}
				}else
					/*############## check foreground cmd ###############*/
					fg_status = exec_fg_cmd(argc, argv);	//run exec and get status value
			}
		}
	}
	return 0;
}


/*###############################################################
							FUNCTIONS
################################################################*/
int check_bg_exec(int argc, char **argv){
	if(argc > 1 && strcmp(argv[argc-1],"&")==0)
		return 1;						// background process detected
	else
		return 0;
}

int exec_fg_cmd(int argc, char **argv){
	int status=0; 					//check status value 0/1
  	pid_t pid;
  	int fd, i;
  	
	pid = fork();
	if(pid == -1){	//error in fork();
		perror("'fork()'");
		exit(1);

	/*########## Child Process ##########*/
	}else if(pid == 0){	
		/*################### check redirection cmd ####################*/
		for(i=0; i<argc; i++){
			if(argc >= 3 && strcmp(argv[i], "<")==0 || strcmp(argv[i], ">")==0){
				if(*argv[1]=='<'){		//stdin
    				fd = open(argv[2], O_RDONLY);
    				if(fd < 0){
						fprintf(stderr, "smallsh: cannot open %s for input\n", argv[2]);
						exit(1);
    				}
					dup2(fd, 0);
					close(fd);		
    			}else if(*argv[1]=='>'){		//stdout
    				fd = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0644);
    				if(fd < 0){
						fprintf(stderr, "smallsh: cannot open %s for input\n", argv[2]);
						exit(1);
    				}
					dup2(fd, 1);
					close(fd);
    			}
    		argv[1] = NULL;		//set redirection to be NULL 
			}
		}
    	//execute command:
    	execvp(argv[0],argv);
    	fprintf(stderr, "%s: No such file or directory\n", argv[0]);
    	fflush(NULL);	
    	exit(1);
  	}
    
	/*########## Parent Process ##########*/
	waitpid(pid, &status, WUNTRACED);	//status indicates last foreground exit status
	return status;
}

void get_fg_status(int status){
	if (WIFEXITED(status)){			//returns true if the child terminated normally
        printf("Exit value %d\n", WEXITSTATUS(status));
        fflush(NULL);
    }else if (WIFSIGNALED(status)) {		//terminated by signal
        printf("Terminated by signal %d\n", WTERMSIG(status));
        fflush(NULL);
    }
}

void IntHandler(int signo){
	printf(" Terminated by signal %d\n", signo);
	fflush(NULL);
	return;
}
