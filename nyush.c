#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <libgen.h>
#include <termios.h>
#include <sys/stat.h>
#include <dirent.h>

char suspended_jobs[100][100];
int suspended_jobs_pid[100];
int num_suspendedd_jobs=0;

int pid1;

char *flg=",,";
/*
void sig_handler(int signum){
   if(getpid()==pid1){
      return;
   }else{ 
      raise(SIGSTOP);
      printf("done");
   }
}
*/

void update_suspended(int x){
   for(int n=x+1;n<num_suspendedd_jobs;n++){
      strcpy(suspended_jobs[n-1],suspended_jobs[n]);
      suspended_jobs_pid[n-1]=suspended_jobs_pid[n];

   }
   

   num_suspendedd_jobs--;
}

void add_suspended(int spid,char arr_str[]){
   strcpy(suspended_jobs[num_suspendedd_jobs],arr_str);
   suspended_jobs_pid[num_suspendedd_jobs]=spid;
   num_suspendedd_jobs++;

}



void ignore_signals()
{
   signal(SIGINT, SIG_IGN);
   signal(SIGQUIT, SIG_IGN);
   signal(SIGTSTP, SIG_IGN);

}

void reset_signals()
{
   signal(SIGINT, SIG_DFL);
   signal(SIGQUIT, SIG_DFL);
   signal(SIGTSTP, SIG_DFL);
}

//

char *find_program(char *program_name) {
    char *path = "/usr/bin/";
    //char *dir = strtok(path, ":");
    
    //while (dir != NULL) {
        char *program_path = malloc(strlen(path) + strlen(program_name) + 2);
        strcpy(program_path, path);
        strcat(program_path, program_name);
        

        struct stat sb;
        if (stat(program_path, &sb) == 0 && sb.st_mode & S_IXUSR) {
            //return program_path;
        }
        return program_path;

        free(program_path);
     //   dir = strtok(NULL, ":");
   // }

   // return NULL;
}

char *locate_program(char *program_name, char *cwd) {
    char *program_path = NULL;

    // Check if the program name is an absolute path
    if (program_name[0] == '/') {
        program_path = strdup(program_name);
        if (access(program_path, X_OK) != 0) {
            free(program_path);
            return flg;
        }
    }
    // Check if the program name is a relative path
    else if (strstr(program_name, "/") != NULL) {
        //get rid of the . at the first 
        
        if(program_name[0]=='.'&&program_name[1]!='.'){
         program_name+=2;
          
        }

        program_path = malloc(strlen(cwd) + strlen(program_name) + 2);
        strcpy(program_path, cwd);
        strcat(program_path,"/");
        strcat(program_path, program_name);
        if (access(program_path, X_OK) != 0) {
            free(program_path);
            return flg;
        }
    }
    // Check if the program name is in /usr/bin
    else {
        program_path = find_program(program_name);
        if (program_path == NULL) {
            return flg;
        }
    }

    return program_path;
}

//




void execute_command(char *arr[], int arr_size){
      int in_fd;
      int out_fd;


      //Check for input/output redirection 
      int red_input=0; int red_output=0; int app_output=0;
      char *inputfile = NULL;
      char *outputfile = NULL;
      for (int i = 0; arr[i] != NULL; i++) {
         if (strcmp(arr[i], "<") == 0) {
            red_input=1;
            inputfile = arr[i+1];
            arr[i]=NULL;
            i++;
      } else if (strcmp(arr[i], ">") == 0) {
            red_output=1;
            outputfile = arr[i+1];
            arr[i]=NULL;
            i++;
      }else if (strcmp(arr[i], ">>") == 0) {
            app_output=1;
            outputfile = arr[i+1];
            arr[i]=NULL;
            i++;
      }
      }
      

      
    
         //Handle input/output redirection
         if(red_input){
            in_fd = open(inputfile, O_RDONLY);
            if (in_fd == -1) {
               fprintf(stderr, "Error: invalid file\n");
               exit(0);
            }

            dup2(in_fd, STDIN_FILENO);
            close(in_fd);

         }
         if(red_output){
            out_fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (out_fd == -1) {
               fprintf(stderr, "Error: invalid file\n");
               exit(0);
            }

            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);

         }
         if(app_output){
            out_fd = open(outputfile, O_WRONLY | O_CREAT | O_APPEND, 0666);
            if (out_fd == -1) {
               fprintf(stderr, "Error: invalid file\n");
               exit(0);
            }

            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);

         }

         reset_signals();
         

         //locating program
         if(arr_size){
            char *cwd = getcwd(NULL, 0);
            char* temp_prog_path=locate_program(arr[0],cwd);
            execv(temp_prog_path,arr);
            fprintf(stderr, "Error: invalid program\n");
            exit(0);

         }else{
            exit(0);
         }
            


         

         
      
      




}



int main(/*int argc, const char *const *argv*/){


   char cwd[256];


   ignore_signals();

   while (1){

   ignore_signals();

   //print prompt
    char* cwd1=getcwd(cwd,sizeof(cwd)); 
    char* cwdbase= basename(cwd);
    if(strcmp(cwd1,"/")!=0) printf("[nyush %s]$ ",cwdbase); else printf("[nyush /]$ ");
    fflush(stdout);

   char input[1000];
   char input1[1000];

   
   if(fgets(input, sizeof(input), stdin)==NULL){
      exit(0);
   }

    //check if command is null
   if (strcmp (input, "\n")==0) {
      continue;
   }

   input[strcspn(input, "\n")] = 0;
   strcpy(input1,input);

   //Build-in commands: 
   char *temp[1000]; int arr_size_temp=0;
   char *temp_element = strtok(input, " ");

    while( temp_element!=NULL ) {
      temp[arr_size_temp] = temp_element; arr_size_temp++;
      temp_element = strtok(NULL, " ");
    }
   //null element to specify the end of array
   temp[arr_size_temp]=NULL;

   

   //check if exit 
   if(strcmp(input, "exit") == 0) {
      if(arr_size_temp>1){
         fprintf(stderr, "Error: invalid command\n");
         continue;
      }else{
         if(num_suspendedd_jobs>0){
            fprintf(stderr, "Error: there are suspended jobs\n");
            continue;
         }else break;
      }
         
   }

   //check if cd 
   if(strcmp(temp[0], "cd") == 0) {
      int chdir_value;
      if(arr_size_temp==2){
         chdir_value=chdir(temp[1]);
         if(chdir_value==-1){
            fprintf(stderr,"Error: invalid directory\n");
         }
      }else{
         fprintf(stderr, "Error: invalid command\n");
       
      }
      continue;

   }
   

    // job
   if(strcmp(input,"jobs")==0){
      if(arr_size_temp>1){
         fprintf(stderr, "Error: invalid command\n");
      }else{
         //list of jobs
         for(int m=0;m<num_suspendedd_jobs;m++){
            //debug
            //printf("[%d] %d\n",m+1,suspended_jobs_pid[m]);
            printf("[%d] %s\n",m+1,suspended_jobs[m]);
         }
      }
      continue;

   }

   //fg
   if(strcmp(temp[0],"fg")==0){
      if(arr_size_temp!=2){
         fprintf(stderr, "Error: invalid command\n");
      }else{
         int x=atoi(temp[1]);
         if(x>num_suspendedd_jobs||x<1){
            fprintf(stderr, "Error: invalid job\n");
         }else{
            int temp3=suspended_jobs_pid[x-1];
            char temp_str3[100]; strcpy(temp_str3,suspended_jobs[x-1]);
            update_suspended(x-1);
            if(kill(temp3,SIGCONT)==-1){
               fprintf(stderr, "errno: %d\n", errno);
               printf("%d\n",kill(temp3, 0));
               
            }
            int status2;
            waitpid(temp3,&status2,WUNTRACED);
            //debug
            //kill(getpid(),SIGCONT);
         
          if(WIFSTOPPED(status2)){
            //add to job
            add_suspended(temp3,temp_str3);
         }
            
         }
   

      }
      continue;
   }
   
   

   //Check for invalid positions of pipe(|) or file handling operators
   if(strcmp(temp[0], "|")==0||strcmp(temp[arr_size_temp-1], "|")==0||strcmp(temp[arr_size_temp-1], ">")==0
   ||strcmp(temp[arr_size_temp-1], ">>")==0||strcmp(temp[arr_size_temp-1], "<")==0){
      fprintf(stderr, "Error: invalid command\n");
      continue;
   }

   //split into separate commands through pipe delimiter
   char *tokens[1000];
   int i = 0;
   tokens[i] = strtok(input1, "|");
   while (tokens[i] != NULL && i < 1000) {
      i++;
      tokens[i] = strtok(NULL, "|");
      
   }
   int num_commands=i-1;
   
   
   //create pipes
   
   int pipe_fds[num_commands][2];
   int child_pids[num_commands];
   for (int j = 0; j < num_commands; j++) {
      int temp_pipe=pipe(pipe_fds[j]);
      if (temp_pipe == -1) {
         perror("pipe failed");
         exit(0);
         }
   }
   

   //Iterate through each command separated by pipes
   for (int j = 0; j <= num_commands; j++) {

      char arr_string[1000];
      strcpy(arr_string,tokens[j]);

      child_pids[j]=fork();
      if(child_pids[j]==-1){
         fprintf(stderr,"Error: Invalid Command\n");
         exit(0);
         break;
      }else if(child_pids[j]==0){
         // Close all pipes in child process
         /*
         for (int k = 0; k < num_commands; k++) {
            close(pipe_fds[k][0]);
            close(pipe_fds[k][1]);
         }
         */
         
         //printf("%d\n",getpid());
         if (j > 0) {
            // Connect stdin to read from previous pipe
            if (dup2(pipe_fds[j - 1][0], STDIN_FILENO) == -1) {
               //perror("dup2 failed");
               exit(0);
            }
            close(pipe_fds[j-1][0]);
            close(pipe_fds[j-1][1]);
         }
      
         if (j < num_commands) {
            // Connect stdout to write to next pipe
            if (dup2(pipe_fds[j][1], STDOUT_FILENO) == -1) {
               //perror("dup2 failed");
               exit(0);
            }
           close(pipe_fds[j][1]);
           close(pipe_fds[j][0]);
         }

         // Close all pipes in child process
         
         for (int k = 0; k < num_commands; k++) {
            if (k != j - 1 && k != j) {
            close(pipe_fds[k][0]);
            close(pipe_fds[k][1]);
            }
         }
         

        //create command array using " " delimiter
        char *arr[1000]; int arr_size=0;
        char *element = strtok(tokens[j], " ");

        while( element!=NULL ) {
           arr[arr_size] = element; arr_size++;
           element = strtok(NULL, " ");
        }
        //null element to specify the end of array
        arr[arr_size]=NULL;

        
        execute_command(arr,arr_size);

      }
   }


  
for (int i = 0; i < num_commands; i++) {
            close(pipe_fds[i][0]);
            close(pipe_fds[i][1]);
         
}

   for (int j = 0; j <= num_commands; j++) {
         int status1;
         waitpid(child_pids[j], &status1, WUNTRACED);
         
          if(WIFSTOPPED(status1)){
            //add to job
            add_suspended(child_pids[j],tokens[0]);
         }
}
}
}



