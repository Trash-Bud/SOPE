#include <sys/types.h>
#ifndef LOGS_H
#define LOGS_H

int check_if_env_var_set();

int create_log_file();

int open_file();

int send_proc_create(long procTimeBegin,char* args[],int num_args);

int close_log_file(int fd);

void display_info_signal(char* filename, int total_files_found, int total_files_processed);

int send_proc_exit(long procTimeBegin,int exit_status);

long get_time_until_now(long procTimeSinceBoot);

long getMillisecondsSinceEpoch();

int send_signal_recv(long procTimeBegin,int signal);

int get_sig_name(int signal, char output[100]);

int send_signal_sent(long procTimeBegin,int signal,pid_t pid);

int send_file_mode_change(long procTimeBegin,int oldPerms, int newPerms, char filename[4096]);

int get_real_file_path(char filename[4096],char realpath[4096]);

double get_double_from_str(char* str,int desired_ind);

long get_long_from_str(char* str,int desired_ind);

#endif