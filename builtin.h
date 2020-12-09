#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <wait.h>
#include <pwd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

typedef struct
{
    char *name;
    Function *func;
    char *doc;
} Builtin_cmd;

int builtin_cd(), builtin_pwd();

int builtin_cd(char *arg)
{
    if (chdir(arg) == -1)
    {
        perror(arg);
        return 1;
    }
    return 0;
}

int builtin_pwd()
{
    char pwd[1024];
    if (getcwd(pwd, sizeof(pwd) - 1) == NULL)
    {
        printf("Error getting pwd!\n");
        return 1;
    }
    else
    {
        printf("%s\n", pwd);
        return 0;
    }
}

//命令表
static Builtin_cmd builtin_cmd_list[] = 
{
    {"cd", builtin_cd, "Change dir"},
    {"pwd", builtin_pwd, "Get current path"}
};

const int builtin_num = (sizeof(builtin_cmd_list) / sizeof(Builtin_cmd));