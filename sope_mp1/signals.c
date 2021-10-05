#include "signals.h"
#include "logs.h"
#include <sys/types.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

extern long timeSinceEpochParentStart;
extern bool write_logs;
extern int child_process_index; 
extern int files_processed;
extern int files_modified;
extern char filename[4096];
extern pid_t child_processes[500];
int wait_times_ms = 30000;

void send_signal(int pid, int signal){
    kill(pid,signal);
}

void signal_handler_default(int signo) {
	if(write_logs) send_signal_recv(timeSinceEpochParentStart,signo);
}

void signal_handler_SIGINT_child(int signo){
    usleep(wait_times_ms);
    if(write_logs) send_signal_recv(timeSinceEpochParentStart,signo);
    display_info_signal(filename,files_processed,files_modified);
    pause();
}

void signal_handler_SIGINT_parent(int signo){
    char answer = 'b';
    pid_t wpid;
    int status = 0;
    if(write_logs) send_signal_recv(timeSinceEpochParentStart,signo);
    display_info_signal(filename,files_processed,files_modified);
    usleep(wait_times_ms * 20);
    
    do{
        printf("Do you wish to resume the program? (y/n) ?: ");
        scanf("%c", &answer);
        printf("you answered: %c \n",answer);
        if(answer == 'y' || answer == 'Y'){
            for(int i = 0; i < child_process_index;i++){
        
                if(write_logs) send_signal_sent(timeSinceEpochParentStart,SIGCONT,child_processes[i]);
                usleep(wait_times_ms);
                send_signal(child_processes[i],SIGCONT); // para continuar o processo
            }
            break;
        }else if(answer == 'n' || answer == 'N'){

            for(int i = 0; i < child_process_index;i++){
                if(write_logs) send_signal_sent(timeSinceEpochParentStart,SIGTERM,child_processes[i]);
                usleep(wait_times_ms);
                send_signal(child_processes[i], SIGTERM);  //matar as crianças
            }
            while((wpid = wait(&status)) > 0){};
            if(write_logs) send_proc_exit(timeSinceEpochParentStart,0);
            exit(0);
        }
        //limpar buffer
        answer = ' ';
    }while(answer != 'y' && answer != 'Y' && answer != 'n' && answer != 'N');

}


void define_handlers_parent(){
    struct sigaction new, new_sigint, old;
    sigset_t smask;	// defines signals to block while func() is running
    // prepare struct sigaction
    if (sigemptyset(&smask)==-1)	// block no signal
      perror("sigsetfunctions");
    new.sa_handler = signal_handler_default;
    new.sa_mask = smask;
    new.sa_flags = 0;	// usually works

    new_sigint.sa_handler = signal_handler_SIGINT_parent;
    new_sigint.sa_mask = smask;
    new_sigint.sa_flags = 0;	// usually works

    if(sigaction(SIGINT, &new_sigint, &old) == -1)
      perror("sigaction");
    if(sigaction(SIGHUP, &new, &old) == -1)
      perror("sigaction");
    if(sigaction(SIGQUIT, &new, &old) == -1)
      perror("sigaction");
    if(sigaction(SIGBUS, &new, &old) == -1)
      perror("sigaction");
    if(sigaction(SIGSEGV, &new, &old) == -1)
      perror("sigaction");
    if(sigaction(SIGPIPE, &new, &old) == -1)
      perror("sigaction");
    if(sigaction(SIGALRM, &new, &old) == -1)
      perror("sigaction");
    if(sigaction(SIGTERM, &new, &old) == -1)
      perror("sigaction");
    if(sigaction(SIGCHLD, &new, &old) == -1)
      perror("sigaction");
}

void signal_handler_SIGCONT(int signo){ //continue execution
    signal_handler_default(signo);
    for(int i =0; i < child_process_index;i++){
        if(write_logs) send_signal_sent(timeSinceEpochParentStart,SIGCONT,child_processes[i]);
        send_signal(child_processes[i],SIGCONT);
        usleep(wait_times_ms);
    }
}

void signal_handler_SIGTERM(int signo){ //kill all children of this process as well
    pid_t wpid;
    int status = 0;
    signal_handler_default(signo);
    for(int i =0; i < child_process_index;i++){
        if(write_logs) send_signal_sent(timeSinceEpochParentStart,SIGTERM,child_processes[i]);
        usleep(wait_times_ms);
        send_signal(child_processes[i],SIGTERM);
    }
    while((wpid = wait(&status)) > 0){};
    if(write_logs) send_proc_exit(timeSinceEpochParentStart,0);
    exit(0);

}


void define_signal_handler_children(){
    struct sigaction new, new_sigint, new_sigcont, new_sigterm, old;
    sigset_t smask;	// defines signals to block while func() is running
    // prepare struct sigaction
    if (sigemptyset(&smask)==-1)	// block no signal
      perror("sigsetfunctions");
    new.sa_handler = signal_handler_default;
    new.sa_mask = smask;
    new.sa_flags = 0;	// usually works

    new_sigint.sa_handler = signal_handler_SIGINT_child;
    new_sigint.sa_mask = smask;
    new_sigint.sa_flags = 0;	// usually works

      //this will be the signal that kids receive to continue execution
    new_sigcont.sa_handler = signal_handler_SIGCONT;
    new_sigcont.sa_mask = smask;
    new_sigcont.sa_flags = 0;

      //this is will the signal that kids receive that means they should die right there
    new_sigterm.sa_handler = signal_handler_SIGTERM;
    new_sigterm.sa_mask = smask;
    new_sigterm.sa_flags = 0;

    if(sigaction(SIGCONT, &new_sigcont, &old) == -1)
      perror("sigaction");

    if(sigaction(SIGINT, &new_sigint, &old) == -1)
      perror("sigaction");
    if(sigaction(SIGTERM, &new_sigterm, &old) == -1)
      perror("sigaction");
    if(sigaction(SIGHUP, &new, &old) == -1)
      perror("sigaction");
    if(sigaction(SIGQUIT, &new, &old) == -1)
      perror("sigaction");
    if(sigaction(SIGBUS, &new, &old) == -1)
      perror("sigaction");
    if(sigaction(SIGSEGV, &new, &old) == -1)
      perror("sigaction");
    if(sigaction(SIGPIPE, &new, &old) == -1)
      perror("sigaction");
    if(sigaction(SIGALRM, &new, &old) == -1)
      perror("sigaction");
    if(sigaction(SIGCHLD, &new, &old) == -1)
      perror("sigaction");
}
