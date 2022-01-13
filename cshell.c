#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>

// A linked list node 
struct Node { 
    char data[1024];
    int pid; 
    struct Node* next; 
};

struct Node* head = NULL; 
struct Node* current = NULL; 
#define TRUE 1
#define FALSE !TRUE

// Shell pid, pgid, terminal modes
static pid_t GBSH_PID;
static pid_t GBSH_PGID;
static int GBSH_IS_INTERACTIVE;
static struct termios GBSH_TMODES;

static char* currentDirectory;
extern char** environ;

struct sigaction act_child;
struct sigaction act_int;

int no_reprint_prmpt;

pid_t pid;

#define LIMIT 256 // max number of tokens for a command
#define MAXLINE 1024 // max number of characters from user input



//Displays the prompt for the shell
void shellPrompt()
{
char host[100];
	strcpy(host,"rushipardeshi@Rushs-Macbook-Pro");
	char cwd[1024];
	char hostn[1204] = "";
	gethostname(hostn, sizeof(hostn));
	getcwd(cwd, sizeof(cwd));
	if (strncmp(cwd,hostn,strlen(hostn))==0){
		char *b = cwd +strlen(hostn);
		printf("<%s:~%s>",host,b);
	}
	else{
		printf("<%s:%s> ",host,cwd);
	}	
}

//Method to change directory (the shell command chdir or cd)
int changeDirectory(char* args[]){
	// If no path written (only 'cd'), then go to the home directory
	if (args[1] == NULL) {
		chdir(getenv("HOME")); 
		return 1;
	}
	// Else change the directory to the one specified by the argument, if possible
	else{ 
		if (chdir(args[1]) == -1) {
			printf(" %s: no such directory\n", args[1]);
            return -1;
		}
	}
	return 0;
}

void append(struct Node** head, char *new_data,int child_pid)
{
    struct Node* new_node = (struct Node*) malloc(sizeof(struct Node));
    struct Node *last = *head;
  strcpy(new_node-> data , new_data);
  new_node->pid=child_pid;
    new_node->next = NULL;
    if (*head == NULL)
    {
    *head = new_node;
    return;
    }
    while (last->next != NULL)
        last = last->next;
    last->next = new_node;
    return;
}



/*
 LAUNCHING THE PROGRAM
*/ 
void launchProg(struct Node** head,struct Node** current,char **args, int background){	 
	 int err = -1;
	 int a=0;
	char line[1024]="";
	
	if(background==1){
	char token[256];
	// strncpy(args[0],args[0]+1,strlen(args[0]));
	}
	
	while (args[a]){
		strcat(line,args[a]);
		strcat(line," ");
		a++;
	}
	 if((pid=fork())==-1){
		 printf("Child process could not be created\n");
		 return;
	 }
	if(pid==0){
		// set the child to ignore SIGINT signals
		signal(SIGINT, SIG_IGN);
		
		setenv("parent",getcwd(currentDirectory, 1024),1);	
		
		if (execvp(args[0],args)==err){
			printf("Command not found!!\n");
			//kill(getpid(),SIGTERM);
		}
	 }
	 else{
	 
	 if (background == 0){
		 waitpid(pid,NULL,0);
		 append(head,line,pid);
	 }else{
		 append(current,line,pid);
		 append(head,line,pid);
		 //printf("Process created with PID: %d\n",pid);
	 }}	 
}



//-----------------------------------------PARSE----------------------------
void parse(char *line, char **args){
    if (strcmp(line, "exit\n") == 0)
            exit(EXIT_SUCCESS);
        char **next = args;
        char *temp = strtok(line, " \n");
        while (temp != NULL)
        {
            *next++ = temp;
            temp = strtok(NULL, " \n");
        }
        *next = NULL;
        for (next = args; *next != 0; next++)
            puts(*next);
}
//----------------------------------------APPEND-------------------------------
//---------------------------------------checking numbers----------------------------------
int digits_only(char *s)
{
    for (int i = 0; i < strlen(s); i++) {
        if ((s[i]<'0' || s[i]>'9')) return 0;
    }

    return 1;
}

//-----------------------------------------Number of nodes------------------------------------
int getCount(struct Node* head) 
{ 
    // Base case 
    if (head == NULL) 
        return 0; 
  
    // count is 1 + count of remaining list 
    return 1 + getCount(head->next); 
} 

//--------------------------------------PRINT-----------------------------------
void printdata(struct Node** head_ref,char* value) {
  struct Node* ptr = *head_ref;
  ///// pid all
  if (strcmp(value,"FULL")==0){
  printf("List of processes with PIDs spawned from this shell:(If no following output then NO Background Process!)\n");
  while(ptr) {
      printf("command name: %s  process id: %d\n",ptr->data,ptr->pid);
      ptr = ptr->next;
  }
  }
  else if (digits_only(value)==1){
    int num=atoi(value);
    int last=0; 
    int count=1;
    int com=getCount(ptr);
    while(ptr) {
      last=getCount(ptr);
      if(last<=num){
         while(ptr) {
      printf("%d %s\n",count,ptr->data);
      count=count+1;
      ptr = ptr->next;
                  } 
          break;
      }
      ptr = ptr->next;

  }
    if (last!=num){
        printf("Only %d commands were executed previously.\n",com);
    }

    }
}

//-------------------------------------FREELIST------------------------------------------------

void freeList(struct Node** head_ref)
{
   struct Node* current = *head_ref;
   struct Node* next;
 
   while (current != NULL) 
   {
       next = current->next;
       free(current);
       current = next;
   }
   *head_ref = NULL;
}

//----------------------------------STANDARD INPUT HANDLER-------------------------------------------
 
int commandHandler(char * args[]){
	int i=0,j = 0;
	int fileDescriptor,standardOut, aux;
	int background = 0;
	char line[1024]="";
	int a=0;
	int child_pid;
	while (args[a]){
		strcat(line,args[a]);
		strcat(line," ");
		a++;
	}
	char *args_aux[256];
	
	while ( args[j] != NULL){
		if ( (strcmp(args[j],">") == 0) || (strcmp(args[j],"<") == 0) || (strcmp(args[j],"&") == 0)){
			break;
		}
		args_aux[j] = args[j];
		j++;
	}
	// 'exit' command quits the shell
	if(strcmp(args[0],"stop") == 0) {
		//append(&head, "exit");
		freeList(&head);
		printf("Freed memory successfully!!\n");
		printf("Exiting Successfully!!\n");
		exit(0);
		}
	//-----------------------------------PID ALL and PID CURRENT----------------------------------
	else if (strcmp("pid",args[0])==0 && args[1]){
		if (strcmp("all",args[1])==0){
		printdata(&head,"FULL");
        }
		else if(strcmp("current",args[1])==0){
		printdata(&current,"FULL");
        }
		append(&head,line,getpid()); 
	  }
	//------------------------------------------------PID--------------------------------------------- 
	else if (strcmp("pid",args[0])==0 ){
		int child_pid;
		printf("command name: ./completecode  process id: %d\n",getpid());
        child_pid=getpid();
		append(&head,line,getpid());
        return child_pid;   
	  }
    
    //--------------------------- 'pwd' command prints the current directory--------------------------
     else if (strcmp(args[0],"pwd") == 0){
         append(&head, "pwd",getpid());
        if (args[j] != NULL){
            if ( (strcmp(args[j],">") == 0) && (args[j+1] != NULL) ){
                fileDescriptor = open(args[j+1], O_CREAT | O_TRUNC | O_WRONLY, 0600);
                standardOut = dup(STDOUT_FILENO);
                dup2(fileDescriptor, STDOUT_FILENO);
                close(fileDescriptor);
                printf("%s\n", getcwd(currentDirectory, 1024));
                dup2(standardOut, STDOUT_FILENO);
            }
        }else{
            printf("%s\n", getcwd(currentDirectory, 1024));
        }
    }
    //-------------------------!HISTN----------------------------------
    else if (strncmp(args[0],"!HIST",5)==0){
        strncpy(args[0],args[0]+5,strlen(args[0]));
        int num=atoi(args[0]);
        struct Node *temp=head;
        int count=1;
        while(temp){
            if (count==num){
            char new_args[MAXLINE]; // buffer for the user input
            char * tokens[LIMIT];
            memset ( new_args, '\0', MAXLINE );
            strcpy(new_args,temp->data);
            if((tokens[0] = strtok(new_args," \n\t")) == NULL) continue;
            int numTokens = 1;
            while((tokens[numTokens] = strtok(NULL, " \n\t")) != NULL) numTokens++;
                commandHandler(tokens);
                break;
            }
            count++;
        temp=temp->next;

        }

    }
        //----------------------------------HISTN------------------------------------------
    else if (strncmp("HIST",args[0],4)==0){
          char num_check[1024];
          strncpy(num_check,args[0]+4,strlen(args[0]));
          if (digits_only(num_check)==1 && strcmp(num_check,"")!=0){
              printdata(&head,num_check);
          }
          else{
              printf("Did you mean HISTN?\nHIST is to be used with number n, where the n commands exected previously will be shown\n");
          }
          child_pid=getpid();
            append(&head,line,child_pid);
            return child_pid;

      }
     // --------------------------------'clear' command clears the screen-------------------------
    else if (strcmp(args[0],"clear") == 0) {
        //append(&head, "clear");
        system("clear");
    }
    // -----------------------'cd' command to change directory-----------------------------------
    else if (strcmp(args[0],"cd") == 0) {
       append(&head, line ,getpid());
    changeDirectory(args);
    }
    //----------------------------for rest command using EXECVP---------------------------------
    else{
        while (args[i] != NULL && background == 0){
            if (strncmp(args[i],"&",1) == 0){
                background = 1;
            }
            // else if (strcmp(args[i],"|") == 0){
            //     pipeHandler(args);
            //     return 1;
            // }
            i++;
        }
        args_aux[i] = NULL;
        launchProg(&head,&current,args_aux,background);
    }
return 1;
}

//----------------------------Method used to manage pipes-----------------------------
void pipeHandler(char * args[]){
	// File descriptors
	int filedes[2]; // pos. 0 output, pos. 1 input of the pipe
	int filedes2[2];
	
	int num_cmds = 0;
	
	char *command[256];
	
	pid_t pid;
	
	int err = -1;
	int end = 0;
	
	// Variables used for the different loops
	int i = 0;
	int j = 0;
	int k = 0;
	int l = 0;
	
	//calculate the number of commands 
	while (args[l] != NULL){
		if (strcmp(args[l],"|") == 0){
			num_cmds++;
		}
		l++;
	}
	num_cmds++;

	while (args[j] != NULL && end != 1){
		k = 0;

		while (strcmp(args[j],"|") != 0){
			command[k] = args[j];
			j++;	
			if (args[j] == NULL){
				end = 1;
				k++;
				break;
			}
			k++;
		}

		command[k] = NULL;
		j++;		
		if (i % 2 != 0){
			pipe(filedes); // for odd i
		}else{
			pipe(filedes2); // for even i
		}
		
		pid=fork();
		
		if(pid==-1){			
			if (i != num_cmds - 1){
				if (i % 2 != 0){
					close(filedes[1]); // for odd i
				}else{
					close(filedes2[1]); // for even i
				} 
			}			
			printf("Child process could not be created\n");
			return;
		}
		if(pid==0){
			if (i == 0){
				dup2(filedes2[1], STDOUT_FILENO);
			}
			else if (i == num_cmds - 1){
				int check=0;
				int output=0;
				while(command[check]!=NULL){
					if (strcmp(command[check],">")==0){
						output=1;
						break;
					}
					check=check+1;	
				}
				if (output==1){
					int paste=check+1;
					command[paste-1]=NULL;
					char filename[256];
					while(command[paste]!=NULL){
						strcat(filename,command[paste]);
						strcat(filename," ");
						command[paste]=NULL;
					}
					int file = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
					dup2(file,STDOUT_FILENO);
					

				}
				if (num_cmds % 2 != 0){ // for odd number of commands
					dup2(filedes[0],STDIN_FILENO);
				}else{ // for even number of commands
					dup2(filedes2[0],STDIN_FILENO);
				}
			}else{ // for odd i
				if (i % 2 != 0){
					dup2(filedes2[0],STDIN_FILENO); 
					dup2(filedes[1],STDOUT_FILENO);
				}else{ // for even i
					dup2(filedes[0],STDIN_FILENO); 
					dup2(filedes2[1],STDOUT_FILENO);					
				} 
			}
			
			if (execvp(command[0],command)==err){
				kill(getpid(),SIGTERM);
			}		
		}
				
		// CLOSING DESCRIPTORS ON PARENT
		if (i == 0){
			close(filedes2[1]);
		}
		else if (i == num_cmds - 1){
			if (num_cmds % 2 != 0){					
				close(filedes[0]);
			}else{					
				close(filedes2[0]);
			}
		}else{
			if (i % 2 != 0){					
				close(filedes2[0]);
				close(filedes[1]);
			}else{					
				close(filedes[0]);
				close(filedes2[1]);
			}
		}
				
		waitpid(pid,NULL,0);
				
		i++;	
	}
}
	
//------------------------------delete node(for pid current in background)-----------------------------
void deleteNode(struct Node** head_ref, int key)
{
    struct Node *temp = *head_ref, *prev;
    if (temp != NULL && temp->pid == key) {
        *head_ref = temp->next; // Changed head
        free(temp); // free old head
        return;
    }
    while (temp != NULL && temp->pid != key) {
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL)
        return;

    prev->next = temp->next;
 
    free(temp); // Free memory
}
 
//--------------------------------------SIGNAL HANDLER signal handler for SIGCHLD------------------------
void signalHandler_child(int p){
	int child;
	int status;
	struct Node *check = current;
	child=waitpid(-1, &status, WNOHANG);
	//printf("%d",child);
	while(check){
		if (check->pid==child){
		printf("The command %s with process id %d is terinated\n",check->data,check->pid);
		deleteNode(&current,child);
		break;	
		}
		check=check->next;
	}	
}

//-------------------------------------Signal handler for SIGINT----------------------------------------------
void signalHandler_int(int p){
	// Sending a SIGTERM signal to the child process
	if (kill(pid,SIGTERM) == 0){
		printf("\nProcess %d received a SIGINT signal\n",pid);
		no_reprint_prmpt = 1;			
	}else{
		printf("\n");
	}
}
//-------------------------------------INTIALIZING function-------------------------------
void init(){
        GBSH_PID = getpid();  
        GBSH_IS_INTERACTIVE = isatty(STDIN_FILENO);  

		if (GBSH_IS_INTERACTIVE) {
			// Loop until in the foreground
			while (tcgetpgrp(STDIN_FILENO) != (GBSH_PGID = getpgrp()))
					kill(GBSH_PID, SIGTTIN);             
	              
	              
	        // Set the signal handlers for SIGCHILD and SIGINT
			act_child.sa_handler = signalHandler_child;
			act_int.sa_handler = signalHandler_int;			
			sigaction(SIGCHLD, &act_child, 0);
			sigaction(SIGINT, &act_int, 0);
			
			// Put in process group
			setpgid(GBSH_PID, GBSH_PID); 
			GBSH_PGID = getpgrp();
			if (GBSH_PID != GBSH_PGID) {
					printf("Error, the shell is not process group leader");
					exit(EXIT_FAILURE);
			}
			// Grab control of the terminal
			tcsetpgrp(STDIN_FILENO, GBSH_PGID);  
			
			// Save default terminal attributes for shell
			tcgetattr(STDIN_FILENO, &GBSH_TMODES);

			// Get the current directory that will be used in different methods
			currentDirectory = (char*) calloc(1024, sizeof(char));
        } else {
                printf("Could not make the shell interactive.\n");
                exit(EXIT_FAILURE);
        }
}
//--------------------------------------Main method-------------------------------------------- 

int main(int argc, char *argv[], char ** envp) {
	//struct Node* head = NULL; 
	char line[MAXLINE]; // buffer for the user input
	char * tokens[LIMIT]; // array for the different tokens in the command
	int numTokens;
		
	no_reprint_prmpt = 0; 	// to prevent the printing of the shell
							// after certain methods
	pid = -10;
	
    printf("### Welcome to my homemade shell\n");
	init();
	environ = envp;

	setenv("shell",getcwd(currentDirectory, 1024),1);

	while(TRUE){

		if (no_reprint_prmpt == 0) shellPrompt();
		no_reprint_prmpt = 0;
		
		memset ( line, '\0', MAXLINE );

		fgets(line, MAXLINE, stdin);
		//if nothing is put into the shell
		if((tokens[0] = strtok(line," \n\t")) == NULL) continue;
		//read and tokenize the command
		numTokens = 1;
		while((tokens[numTokens] = strtok(NULL, " \n\t")) != NULL) numTokens++;
		//execute the command
		commandHandler(tokens);
		
	}          

	exit(0);
}
