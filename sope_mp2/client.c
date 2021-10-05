#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "delay.h"
#include "common.h"

int unic_num = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
bool fifo_is_closed = false;

struct thread_param
{
	time_t begin;
    int seconds_to_run;
    char* fifoname;
};

struct fifo_arg{
    int op;
	time_t begin;
	int seconds_to_run;
    int file_descriptor;
};


void stdout_from_fifo(struct Message* msg,char * operation){
    fprintf(stdout,"%ld; %d; %d; %d; %ld; %d; %s \n",time(NULL), msg->rid, msg->tskload, msg->pid, msg->tid,msg->tskres, operation);
}

void read_from_fifo(char* private_fifo_name,time_t begin, int time_out, struct Message* request_msg ){
	int np;

	while ((np = open(private_fifo_name, O_RDONLY)) < 0){
        continue;
    }

    int seconds_elapsed = 0;

	struct Message *msg = malloc(sizeof(struct Message));
	int r;

	do{
		r = read(np, msg, sizeof(struct Message));
        seconds_elapsed = time(NULL) - begin;
        
        //the thread didn't receive an answer from the server in time
        if(seconds_elapsed > time_out){
			stdout_from_fifo(request_msg, "GAVUP");
			close(np);
            remove(private_fifo_name);
            return;
		}
	}while(r == 0);

    //printf("msg->tskres: %d \n", msg->tskres);
	if(msg->tskres == -1){
        fifo_is_closed = true;
        stdout_from_fifo(request_msg,"CLOSD");
    }
    else{
        stdout_from_fifo(msg,"GOTRS");
    }
    
    remove(private_fifo_name);
	free(msg);
	close(np);
	return;
}

void* send_to_fifo(void* arg){
    struct fifo_arg *fifo_args = (struct fifo_arg *) arg;
	int np = fifo_args->file_descriptor;
	
	pthread_t id_thread = pthread_self();

    char* private_fifo_name = (char *) malloc(50 * sizeof(char));
    sprintf(private_fifo_name, "/tmp/%d.%ld",getpid(), id_thread);

    if (mkfifo(private_fifo_name,0666) < 0) {
        perror("mkfifo");
    }

    struct Message* msg = malloc(sizeof(struct Message));
    msg->tid = id_thread;
    msg->tskload = fifo_args->op;
    msg->pid = getpid();
    msg->tskres = -1;

    pthread_mutex_lock(&mutex);
	unic_num++;
    msg->rid = unic_num;
    pthread_mutex_unlock(&mutex);
    stdout_from_fifo(msg,"IWANT");
	int write_output = write(np, msg, sizeof(struct Message));
    if(write_output < 0){
        remove(private_fifo_name);
        free(private_fifo_name);
        fifo_is_closed = true;
        free(msg);
        pthread_exit(NULL);
    } 

    read_from_fifo(private_fifo_name,fifo_args->begin, fifo_args->seconds_to_run,msg);
    
    remove(private_fifo_name);
	free(private_fifo_name);
    free(msg);

	pthread_exit(NULL);
}

void *thread_create(void* arg){

    struct thread_param *param = (struct thread_param *) arg;
    pthread_t ids[1000];
    size_t num_of_threads = 0;
    struct fifo_arg fn;
    double seconds_elapsed = 0;
    
    fn.file_descriptor = -1;

	fn.begin = param->begin;
	fn.seconds_to_run = param->seconds_to_run;
    
    //wait until the server has opened the public fifo before starting request thread creation
    while(seconds_elapsed < param->seconds_to_run){
        fn.file_descriptor = open(param->fifoname, O_WRONLY);
        
        if(fn.file_descriptor != -1){
            break;
        }
        seconds_elapsed = time(NULL) - param->begin;
    }
    

	while(seconds_elapsed < param->seconds_to_run){
        if (!fifo_is_closed){
            fn.op = rand() % 9 + 1; //carga da tarefa 
            pthread_create(&ids[num_of_threads], NULL, send_to_fifo, &fn);
            num_of_threads++;
            usleep(1000 * 50);
		}
        else{

            fn.file_descriptor = open(param->fifoname, O_WRONLY);
            if(fn.file_descriptor != -1){
                fifo_is_closed = false;
            }
        }
        seconds_elapsed = time(NULL) - param->begin;
      
	}

    for(int i = 0; i< num_of_threads; i++) {
	    pthread_join(ids[i], NULL);	
	}
    close(fn.file_descriptor);
    pthread_exit(NULL);
}


int main(int argc, char* argv[]){
   
    time_t begin = time(NULL);
    srand(time(NULL));
    
    //argv[1] should be "-t"
    //argv[2] should be an int
    if(argc != 4){
        printf("Invalid number of arguments! Usage: './c -t <no. of seconds> <public fifoname>' \n");
        return 0;
    }
    if(strcmp(argv[1],"-t") != 0){
        printf("Missing -t flag. Usage: './c -t <no. of seconds> <public fifoname>' \n");
        return 0;
    }

    int seconds_to_run = atoi(argv[2]);

    if(seconds_to_run <= 0){
        printf("Time should be an integer greater than 0. Usage: './c -t <no. of seconds> <public fifoname>' \n");
        return 0;
    }

    pthread_t c0;
    
    char* fifoname = (char *) malloc(50 * sizeof(char));
    sprintf(fifoname,"%s",argv[3]);
    
         
    struct thread_param param;
    param.begin = begin;
    param.seconds_to_run = seconds_to_run;
    param.fifoname = fifoname;
    
    
    pthread_create(&c0, NULL, thread_create, &param);
    
    pthread_join(c0,NULL);
    
	free(fifoname);
    
	return 0;
    
}

