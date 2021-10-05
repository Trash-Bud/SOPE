#include "logs.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <stdbool.h>

long timeSinceEpochParentStart;
bool write_logs;

int check_if_env_var_set(){
    if(getenv("LOG_FILENAME") == NULL){
        return -1;
    }
    return 0;
}

int create_log_file(){

    int fd = open(getenv("LOG_FILENAME"),O_TRUNC | O_CREAT | O_WRONLY,  S_IRWXU);
    if(fd == -1){
        perror("xmod: cannot access the log file: ");
		return -1;
	}
	close_log_file(fd);
    return 0;
};

int open_file(){
    int fd = open(getenv("LOG_FILENAME"),O_APPEND | O_WRONLY);
    if(fd == -1){
        perror("xmod: cannot access the log file: ");
		return -1;
	}
    return fd;
}

int close_log_file(int fd){
    if( close(fd) < 0){
        perror("xmod: cannot access the log file: ");
        return -1;
    }
    return 0;
}

void upcase(char *s)
{
    while (*s)
    {
        *s = toupper(*s);
        s++;        
    }
}

int get_sig_name(int signal, char output[40]){
    char *str = strsignal(signal);
    //can't fail because the possible signals were deffined by us and are all valid
    snprintf(output, 40, "Signal %s", str);
    
    return 0;

}

double get_double_from_str(char* str,int desired_ind){
  char * pch;
  pch = strtok(str," ");
  int index = 0;
  while (pch != NULL)
  {
    if(index == desired_ind) {
        return atof(pch);
    }
    pch = strtok(NULL, " ");
    index++;
  }
  return -1;
}

long get_long_from_str(char* str,int desired_ind){
  char * pch;
  pch = strtok(str," ");
  int index = 0;
  while (pch != NULL)
  {
    if(index == desired_ind) {
        return atol(pch);
    }
    pch = strtok(NULL, " ");
    index++;
  }
  return -1;
}

long getMillisecondsSinceEpoch(){
    struct timeval tv;
    long msecs_time_now;
    if(gettimeofday(&tv,NULL) == -1){
		printf("xmod: could not get time of day");
		return -1;
	}
	msecs_time_now = (tv.tv_sec * 1000) + (tv.tv_usec / 1000); //how many milliseconds has it been since Epoch
    return msecs_time_now;
}

long get_time_until_now(long procTimeSinceBoot){
	return getMillisecondsSinceEpoch() - procTimeSinceBoot;
}

int send_proc_create(long procTimeBegin, char* args[],int num_args){
	int fd = open_file();
    if(fd == -1) return -1;
	long time_elapsed = get_time_until_now(procTimeBegin);
    char* output = malloc(sizeof(char) * 50);
    char final_output[5000];
    snprintf(output, 50, "%ld", time_elapsed);
    strcpy(final_output,output);
    strcat(final_output," ; ");
    snprintf(output,50,"%d",getpid());
    strcat(final_output, output);
    strcat(final_output," ; ");
    strcat(final_output, "PROC_CREAT");
    strcat(final_output," ; ");
    for(int i = 0; i < num_args;i++){
        strcat(final_output,args[i]);
        strcat(final_output," ");
    }

    strcat(final_output,"\n");
    free(output);
    write(fd,final_output,strlen(final_output));
    return close_log_file(fd);
}

int send_proc_exit(long procTimeBegin,int exit_status){
	int fd = open_file();
    if(fd == -1) return -1;
    char output[50];
    long time_elapsed = get_time_until_now(procTimeBegin);
    char final_output[5000];
    snprintf(output, 50, "%ld", time_elapsed);
    strcpy(final_output,output);
    strcat(final_output," ; ");
    snprintf(output,50,"%d",getpid());
    strcat(final_output,output);
    strcat(final_output," ; ");
    strcat(final_output,"PROC_EXIT");
    strcat(final_output," ; ");
    snprintf(output, 50, "%d", exit_status);
    strcat(final_output,output);
    strcat(final_output,"\n");
    write(fd,final_output,strlen(final_output));
    return close_log_file(fd);
}

void display_info_signal(char* filename, int total_files_found, int total_files_processed){
    printf("\n%d ; %s ; %d ; %d \n", getpid(), filename,total_files_found,total_files_processed);
}

int send_signal_recv(long procTimeBegin,int signal){
    int fd = open_file();
    if(fd == -1) return -1;
    char str[50];
    get_sig_name(signal,str);
    long time_elapsed = get_time_until_now(procTimeBegin);
    char output[50];
    char final_output[5000];
    snprintf(output, 50, "%ld", time_elapsed);
    strcpy(final_output,output);
    strcat(final_output," ; ");
    snprintf(output,50,"%d",getpid());
    strcat(final_output,output);
    strcat(final_output," ; ");
    strcat(final_output,"SIGNAL_RECV");
    strcat(final_output," ; ");
    strcat(final_output, str);
    strcat(final_output, "\n");
    write(fd,final_output,strlen(final_output));
    return close_log_file(fd);

}

int send_signal_sent(long procTimeBegin,int signal,pid_t pid){
    int fd = open_file();
    if(fd == -1) return -1;
    char str[40];
    get_sig_name(signal,str);
    long time_elapsed = get_time_until_now(procTimeBegin);
    char output[50];
    char final_output[5000];
    snprintf(output, 50, "%ld", time_elapsed);
    strcpy(final_output,output);
    strcat(final_output," ; ");
    snprintf(output,50,"%d",getpid());
    strcat(final_output,output);
    strcat(final_output," ; ");
    strcat(final_output,"SIGNAL_SENT");
    strcat(final_output," ; ");
    snprintf(output, 50, "%s : %d", str,pid);
    strcat(final_output,output);
    strcat(final_output,"\n");
    write(fd,final_output,strlen(final_output));
    return close_log_file(fd);
}

int get_real_file_path(char filename[4096],char real_path[4096]){
    strcpy(real_path,realpath(filename, NULL)); 
    if (real_path == NULL){
        return -1;        
    }
    return 0;
}

int send_file_mode_change(long procTimeBegin,int oldPerms, int newPerms,char filename[4096]){
    int fd = open_file();
    if(fd == -1) return -1;
    long time_elapsed = get_time_until_now(procTimeBegin);
    char output[50];
    char final_output[5000];
    snprintf(output, 50, "%ld", time_elapsed);
    strcpy(final_output,output);
    strcat(final_output," ; ");
    snprintf(output,50,"%d",getpid());
    strcat(final_output,output);
    strcat(final_output," ; ");
    strcat(final_output,"FILE_MODF");
    strcat(final_output," ; ");
    char real_path[4096];
    if(get_real_file_path(filename,real_path) == 0){
        strcat(final_output,real_path);
    }else{
        strcat(final_output,filename);
    }
    snprintf(output,50," : %o : %o", oldPerms,newPerms);
    strcat(final_output,output);
    strcat(final_output,"\n");
    write(fd,final_output,strlen(final_output));
    return close_log_file(fd);

}