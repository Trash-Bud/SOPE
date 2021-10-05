#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>

/// Creates an array with the valid combination of rwx operations
void build_Perms(struct Perms** perms_arr,int length){
    *perms_arr = malloc(length * sizeof(struct Perms));
    if(*perms_arr == NULL) return;

    struct Perms mode1;
    mode1.perm[0] = 'r';
    mode1.perm[1] = '\0';
    mode1.octal_mode = 04;
    (*perms_arr)[0] = mode1;

    struct Perms mode2;
    mode2.perm[0] = 'w';
    mode2.perm[1] = '\0';
    mode2.octal_mode = 02;
    (*perms_arr)[1] = mode2;

    struct Perms mode3;
    mode3.perm[0] = 'x';
    mode3.perm[1] = '\0';
    mode3.octal_mode = 01;
    (*perms_arr)[2] = mode3;

    struct Perms mode4; 
    mode4.perm[0]=  'r';
    mode4.perm[1]=  'w';
    mode4.perm[2]=  '\0';
    mode4.octal_mode = 06;
    (*perms_arr)[3] = mode4;

    struct Perms mode5;
    mode5.perm[0]=  'r';
    mode5.perm[1]=  'x';
    mode5.perm[2]=  '\0';
	mode5.octal_mode = 05;
	(*perms_arr)[4] = mode5;

    struct Perms mode6; 
    mode6.perm[0]=  'r';
    mode6.perm[1]=  'w';
    mode6.perm[2]=  'x';
    mode6.perm[3]=  '\0';
    mode6.octal_mode = 07;
    (*perms_arr)[5] = mode6;

    struct Perms mode7; 
    mode7.perm[0]=  'w';
    mode7.perm[1]=  'x';
    mode7.perm[2]=  '\0';
    mode7.octal_mode = 03;
    (*perms_arr)[6] = mode7;
}



void print_changes_command(int oldPerms,int newPerms,char filename[4096]){
    char oldPermsString[9];
	char newPermsString[9];
	getPermsStringFormat(oldPerms, oldPermsString);

    printf("mode of '%s' changed from %03o (%s)", filename, oldPerms, oldPermsString);

    getPermsStringFormat(newPerms, newPermsString);

	printf(" to %03o (%s)\n", newPerms,newPermsString);
}

void print_verbose_retain_command(int oldPerms,char filename[4096]){
    char oldPermsString[9];
	getPermsStringFormat(oldPerms, oldPermsString);
	printf("mode of '%s' retained as %o (%s)\n", filename, oldPerms,oldPermsString);
}


void getPermsStringFormat(int perm, char str[9]){
    str[0] = (perm & S_IRUSR) ? 'r' : '-';
    str[1] = (perm & S_IWUSR) ? 'w' : '-';
    str[2] = (perm & S_IXUSR) ? 'x' : '-';
    str[3] = (perm & S_IRGRP) ? 'r' : '-';
    str[4] = (perm & S_IWGRP) ? 'w' : '-';
    str[5] = (perm & S_IXGRP) ? 'x' : '-';
    str[6] = (perm & S_IROTH) ? 'r' : '-';
    str[7] = (perm & S_IWOTH) ? 'w' : '-';
    str[8] = (perm & S_IXOTH) ? 'x' : '-';  
}


//returns 1 if the file is a directory and 2 if it is a regular file
int is_regular_file(const char *path)
{
    struct stat path_stat;
    if(stat(path, &path_stat) == -1){
        return -1;
    };
    if(S_ISDIR(path_stat.st_mode)){
        return 1;
    };
    if(S_ISREG(path_stat.st_mode)){
        return 2;
    }
    return 0;
}

int is_valid_mode(char* mode){
    int index = 0;
    if(atoi(mode) > 777){
        return -1;
    }
    while(mode[index] != '\0'){
        switch(mode[index]){
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            //nothing happens number is valid
            break;
            default:
                return -1;
        }
        index++;
    }
    return 0;
}

int getCurrentPerms(char* file){
    struct stat st;
    int perms = 0;
    int *modeval = malloc(sizeof(int) * 9);
    if(stat(file, &st) == 0){
        mode_t perm = st.st_mode;
        modeval[0] = (perm & S_IRUSR) ? 0400 : 0;
        modeval[1] = (perm & S_IWUSR) ? 0200 : 0;
        modeval[2] = (perm & S_IXUSR) ? 0100 : 0;
        modeval[3] = (perm & S_IRGRP) ? 040 : 0;
        modeval[4] = (perm & S_IWGRP) ? 020 : 0;
        modeval[5] = (perm & S_IXGRP) ? 010 : 0;
        modeval[6] = (perm & S_IROTH) ? 04 : 0;
        modeval[7] = (perm & S_IWOTH) ? 02 : 0;
        modeval[8] = (perm & S_IXOTH) ? 01 : 0;  
    } 
    for(int i = 0; i < 9; i++){
		perms = perms | modeval[i];
    }
   
    return perms;
}

