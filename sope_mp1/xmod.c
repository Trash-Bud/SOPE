#include "signals.h"
#include "utils.h"
#include "logs.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdbool.h>

extern long timeSinceEpochParentStart;
extern bool write_logs;
int child_process_index = 0; 
int files_processed;
int files_modified;
char filename[4096];
pid_t child_processes[500];
int wait_time = 20000;

int set_changes_mode_str(char* str, char* file, int oldPerms){
    struct Perms* perms_arr;
    int length = 7;
    build_Perms(&perms_arr,length);
    if(str[0] != 'u' && str[0] != 'g' && str[0] != 'o' && str[0] != 'a' ){
        return -1;
    }
    if(str[1] != '=' && str[1] != '-' && str[1] != '+'){
        return -1; //command is formmatted incorectly
    }

    int perms = -1;
    int index = 2;
    char mode[4];
    while(str[index] != '\0'){
        if(index == 5) return -1; //invalid mode
        mode[index - 2] = str[index];
        index++;
    }
    mode[index-2] = '\0';
    //iterar pelo array de perms possiveis e verificar se o mode está lá dentro
    for(int i = 0; i < length;i++){
        if(strcmp(perms_arr[i].perm,mode) == 0 ){
            perms = perms_arr[i].octal_mode;
            break;
        }
        else if (i == length -1){
            return -1;
        }
    }
    if(str[0] == 'u') perms = perms << 6;
    else if(str[0] == 'g') perms = perms << 3;
    else if(str[0] == 'a') perms = perms <<6 | perms <<3 | perms;
    if(perms == -1){
		return -1;
    }
    if(str[1] == '+'){
        //add the new permissions to already existant ones
        //lets read the current perms
		perms |= oldPerms;
	}else if(str[1] == '-'){
		perms ^= oldPerms;
    }

	free(perms_arr);
    return perms;
}


int process_file(char* filename, char* mode, bool verbose, bool changes){
    int oldPerms = getCurrentPerms(filename);
    int newPerms;
    if(atoi(mode) == 0 && strcmp(mode,"000") != 0 && strcmp(mode,"0000") != 0){  //if atoi fails, then its a mode specified with letters
        newPerms = set_changes_mode_str(mode,filename,oldPerms);
        if(newPerms == -1){
            printf("xmod: invalid mode: ‘%s’ \n",mode);
            return -1;
        }
    }
    else{
        if(is_valid_mode(mode) == -1){
            printf("xmod: invalid mode: ‘%s’ \n",mode);
            if(write_logs) send_proc_exit(timeSinceEpochParentStart,-1);
            return -1;
    }
    newPerms = strtoll(mode,NULL,8);
    //check if its a valid mode
    if(newPerms == -1){
        printf("xmod: invalid mode: ‘%s’ \n",mode);
        if(write_logs) send_proc_exit(timeSinceEpochParentStart,-1);
        return -1;
        }
    }
    if (chmod(filename, newPerms) != 0){
        perror("xmod failed: ");
        if(write_logs) send_proc_exit(timeSinceEpochParentStart,-1);
        return -1;
    }
    files_modified++;
    
    if(write_logs && oldPerms != newPerms) send_file_mode_change(timeSinceEpochParentStart,oldPerms,newPerms,filename);

    if((changes || verbose) && oldPerms != newPerms){
        print_changes_command(oldPerms,newPerms,filename);
	}
    if(verbose && oldPerms == newPerms){
        print_verbose_retain_command(oldPerms,filename);
    }
    return 0;
}


int search_dir(char* dir,char* mode,bool verbose,bool changes){
     struct dirent *de;  // Pointer for directory entry     
    // opendir() returns a pointer of DIR type.  
    DIR *dr = opendir(dir); 
    if (dr == NULL)  // opendir returns NULL if couldn't open directory 
    { 
        printf("xmod: cannot access '%s': No such file or directory\n", dir); 
        return -1; 
    } 
  
    // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html 
    // for readdir() 

    while ((de = readdir(dr)) != NULL){
        files_processed++;
        if(de->d_type == DT_DIR){ //if path is directory
              continue;
        }
        else if(de->d_type == DT_REG){
            char * filename = malloc(4096 * sizeof(char));
            strcpy(filename,dir);
            strcat(filename,"/");
            strcat(filename,de->d_name);
            process_file(filename,mode, verbose,changes);
              
            free(filename);  
        }   
    }
    return 0; 
}


int search_dir_recursive(char* args[],int arg_num, bool verbose, bool changes){
    struct dirent *de;  // Pointer for directory entry     
    // opendir() returns a pointer of DIR type.  
    char* dir = args[arg_num -1];
    DIR *dr = opendir(dir);
    if (dr == NULL)  // opendir returns NULL if couldn't open directory 
    { 
        printf("xmod: cannot access '%s': No such file or directory\n", dir); 
        return -1; 
    } 
    
    // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html 
    // for readdir() 

    while ((de = readdir(dr)) != NULL){
        files_processed++;
        if(de->d_type == DT_DIR){ //if path is directory
            if(de->d_name[0] != '.'){
                //se não é nem o current_dir nem o pai do curr_dir
                //criar novo processo para dar parse às files deste dir
                //printf("process with pid: %d running on dir: %s is about to cause a fork for dir %s\n\n",getpid(), dir, de->d_name);
                pid_t id = fork();
            
                switch (id) {
                    case -1:
                        perror ("fork"); 
                        exit (1);
                    case 0: //child process
                        // chamar search_dir() com novo   
                        {
                            usleep(wait_time);
                            char* curr_dir = malloc(4096 * sizeof(char));
                            //build the new directory name
                            strcpy(curr_dir,dir);
                            strcat(curr_dir,"/");
                            strcat(curr_dir,de->d_name);
                            
                            char* args_to_exec[arg_num+1];
                            for(int i = 0; i < arg_num;i++){
                                args_to_exec[i] = args[i];
                            }
                            args_to_exec[arg_num - 1] = curr_dir;
                            args_to_exec[arg_num] = NULL;
                            execvp(args[0],args_to_exec);
                            printf("execvp call failed\n"); 
						    return -1;
                        }
						break;
                    default: //parent process
                        usleep(wait_time);
                        child_processes[child_process_index] = id;
                        child_process_index++;
						break;
				}
            }    
        }
        else if(de->d_type == DT_REG){
            char * filename = malloc(4096 * sizeof(char));
            strcpy(filename,dir);
            strcat(filename,"/");
            strcat(filename,de->d_name);
            
            if(process_file(filename,args[arg_num - 2], verbose,changes)) return -1;
            free(filename);  
        }   
    }
    
    return 0; 
}

//this approach was attempted but in the end uptime only has 2 decimal places for milliseconds
//meaning that all values would be in 10 ms intervals so it was deemed not precise enough
// instead we will simply store milliseconds since epoch in env var
//and then when we need to time stuff we simply see the difference between milliseconds now and in the env_var;

/*
long getProcTimeSinceBoot(){
    char begin_str[10];
    sprintf(begin_str, "%d", getpid());
    char proc_file_path[256] = "";
    strcat(proc_file_path,"/proc/");
    strcat(proc_file_path,begin_str);
    strcat(proc_file_path,"/stat");  
    FILE *fp = fopen(proc_file_path, "r");

    char* line = NULL;
    size_t len = 32;
    size_t read;
    line = (char *) malloc(len * sizeof(char));

    long procTimeSinceBoot;
    while ((read = getline(&line, &len, fp)) != -1)
	{
        procTimeSinceBoot = get_long_from_str(line,21);
	}
    procTimeSinceBoot = (long)((double)procTimeSinceBoot / sysconf(_SC_CLK_TCK) * 1000 );
    return procTimeSinceBoot;
}
*/

int main(int argc, char *argv[]){
    write_logs = false;
    files_processed = 0;
    files_modified = 0;
    if (check_if_env_var_set() == 0)
	{
		write_logs = true;
	}
    if(argc < 3){
        printf("xmod: missing operand \n");
        if(write_logs){
            create_log_file();
            send_proc_create(timeSinceEpochParentStart,argv,argc);
            send_proc_exit(timeSinceEpochParentStart,-1);
        } 
        return -1;
    }
    
	if(getenv("PARENT_TIME") == NULL){
        
        //saving the inicial processes procTimeSinceLinuxBoot
        define_handlers_parent();
        timeSinceEpochParentStart = getMillisecondsSinceEpoch();
        char begin_str[20];
        sprintf(begin_str, "%ld", timeSinceEpochParentStart);
    
		setenv("PARENT_TIME", begin_str, 0);

		if (write_logs){
			if(create_log_file() == -1) return -1;
		}
	}else{
        define_signal_handler_children();
        timeSinceEpochParentStart = atol(getenv("PARENT_TIME"));
    }

    if (write_logs){
		send_proc_create(timeSinceEpochParentStart,argv,argc);
    }
    
    char mode[120];

    strcpy(mode,argv[argc - 2]);
    strcpy(filename,argv[argc - 1]);

    bool verbose = false, changes = false, recursive = false;
    
    pid_t wpid;
    int status = 0;

	//check for flags "-v", "-r", "-c"    //check for flags "-v", "-r", "-c"
    // -c - displays only if there were changes in file perms
    // -v - always displays even if nothing changes
    // -r - recursive
    for(int i = 1; i < argc - 2; i++){ //iterate until argc - 2 which specifies mode/octal-mode
        if(strcasecmp("-v",argv[i]) == 0){
            verbose = true;
        }
        else if(strcasecmp("-r",argv[i]) == 0){
            recursive = true;
        }
        else if(strcasecmp("-c",argv[i]) == 0){
            changes = true;
        }
        else{
            while ((wpid = wait(&status)) > 0);

            printf("xmod: invalide option '%s': make sure you write the command in the format 'xmod [options] mode filename/dir' \n", argv[i]);
            if(write_logs) send_proc_exit(timeSinceEpochParentStart,-1);
            return -1;
        }
    }
    
    //check if file exists
    if(is_regular_file(filename) == -1) { //whatever we typed in does not exist
        printf("xmod: cannot access '%s': No such file or directory\n",filename);
        printf("failed to change mode of '%s' from 0000 (---------) to 0000 (---------)\n",filename);
        while ((wpid = wait(&status)) > 0);

        if(write_logs) send_proc_exit(timeSinceEpochParentStart,-1);
        return -1;
    }
    else if(is_regular_file(filename) == 1){ //it is a directory
       if(recursive){
			if(search_dir_recursive(argv,argc,verbose,changes) != 0){
                while((wpid = wait(&status)) > 0){};  
                if(write_logs) send_proc_exit(timeSinceEpochParentStart,-1);
                return -1;
            }
        }
        else{
            if(search_dir(filename,mode,verbose,changes) != 0){
                while((wpid = wait(&status)) > 0){};
                if(write_logs) send_proc_exit(timeSinceEpochParentStart,-1);
                return -1;
            }
		}
    }
    else if(is_regular_file(filename) == 2){ //is a file
        files_processed++;
        if(process_file(filename,mode,verbose,changes) == -1){
            while((wpid = wait(&status)) > 0){};
	        if(write_logs) send_proc_exit(timeSinceEpochParentStart,0);
			return -1;
		}
    }
 
    //tell parent to wait for children
     //sleep used for testing ctrl+c handling
    
    while((wpid = wait(&status)) > 0){};
	if(write_logs) 
        send_proc_exit(timeSinceEpochParentStart,0);
    return 0;
}