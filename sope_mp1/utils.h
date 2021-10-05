#ifndef UTILS_H
#define UTILS_H


struct Perms{
    char perm[4];
    int octal_mode;
};

void build_Perms(struct Perms** perms_arr,int length);

void getPermsStringFormat(int perm, char str[9]);

void print_changes_command(int oldPerms,int newPerms,char filename[4096]);

void print_verbose_retain_command(int oldPerms,char filename[4096]);

int is_valid_mode(char* mode);

int getCurrentPerms(char* file);

int is_regular_file(const char *path);

#endif