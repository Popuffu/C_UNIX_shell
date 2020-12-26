#include "builtin.h"
#include "parse.h"
#include "def.h"
#include "externs.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <wait.h>
#include <linux/limits.h>
#include <pwd.h>
#include <readline/readline.h>
#include <readline/history.h>

typedef struct builtin_cmd
{
    char *name;
    Function *func;

} Builtin_cmd;

int builtin_exit();
int builtin_cd(char *arg);
int builtin_jobs();
int builtin_kill(char *arg);
int builtin_pwd();
int builtin_bg();
int builtin_fg();
int builtin_addpath(char *arg);
int builtin_envpath();
int builtin_history();
char *line_read = NULL;  //终端输入字符串
char *line_trim = NULL; //剔除前端空格的输入字符串

Builtin_cmd builtin_cmd_list[] = // 内置命令表
{
    {"cd", builtin_cd},
    {"pwd", builtin_pwd},
    {"exit", builtin_exit},
    {"jobs", builtin_jobs},
    {"kill", builtin_kill},
    {"bg", builtin_bg},
    {"fg", builtin_fg},
    {"addpath", builtin_addpath},
    {"envpath", builtin_envpath},
    {"history", builtin_history},
    {NULL, NULL}
};

int can_find_pid_node(int pid) // 在进程信息链表中找到对应pid的节点并更改其是否后台的属性
{
    NODE *bng = head->next;
    while (bng != NULL)
    {
        if (bng->npid == pid)
            return 1;
        bng = bng->next;
    }
    return 0;
}

void find_pid_node_and_delete(int pid) // 找到进程信息链表中对应pid的节点并删除
{
    NODE *bng = head->next;
    NODE *pre = head;
    int flag = 0;
    while (bng != NULL)
    {
        if (bng->npid == pid)
        {
            NODE *nxt = bng->next;
            pre->next = nxt;
            flag = 1;
            break;
        }
        pre = bng;
        bng = bng->next;
    }
    if (flag)
        free(bng);
}

void delete_died_node() // 遍历进程信息链表，把所有已经死掉的进程节点删掉
{
    int status;
    NODE *bng = head->next;
    NODE *pre = head;
    while (bng != NULL)
    {
        if (waitpid(bng->npid, &status, WNOHANG) != 0)
        {
            //printf("I free %d\n", bng->npid);
            NODE *nxt = bng->next;
            pre->next = nxt;
            free(bng);
            bng = nxt;
        }
        else
        {
            pre = bng;
            bng = bng->next;
        }
    }
}

void find_pid_node_and_change_backgnd(int pid, int backgnd) // 在进程信息链表中找到对应pid的节点并更改其是否后台的属性
{
    NODE *bng = head->next;
    while (bng != NULL)
    {
        if (bng->npid == pid)
            bng->backgnd = backgnd;
        bng = bng->next;
    }
}

void find_pid_node_and_change_running(int pid, int running) // 在进程信息链表中找到对应pid的节点并更改其是否在运行的属性
{
    NODE *bng = head->next;
    while (bng != NULL)
    {
        if (bng->npid == pid)
            bng->running = running;
        bng = bng->next;
    }
}

int builtin_cd(char *arg) //cd内置命令，切换路径
{
    if (chdir(arg) == -1)
    {
        perror(arg);
        return 1;
    }
    return 0;
}

int builtin_pwd() // pwd内置命令，查看当前路径
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

int builtin_exit() // exit内置命令，退出yaush。如果此时有任务还在运行则输出提示
{
    delete_died_node();
    int Pgnum = 0;
    NODE *tmp = head->next;
    while (tmp != NULL)
    {
        Pgnum++;
        tmp = tmp->next;
    }
    if (Pgnum != 0)
    {
        printf("There are programs in the background,are you sure to exit? y/n\n");
        char c = getchar();
        char tmpc;
        fflush(stdin);
        while ((tmpc = getchar()) != EOF && tmpc != '\n') // 清空输入缓冲区
            ;
        if (c == 'y')
            exit(0);
        else
            return 0;
    }
    else
    {
        exit(0);
    }
    
}

int builtin_kill(char *arg) // kill内置命令，杀死对应pid的进程
{
    int num = 0;
    int i = 0;
    if (strlen(arg) == 0)
        return 0;
    for (i = 0; i < strlen(arg); i++)
        if (!isdigit(arg[i]))
        {
            printf("Can not kill pid: %s!\n", arg);
            return 0;
        }
    num = atoi(arg);
    //printf("I want to kill %d\n", num);
    signal(SIGQUIT, SIG_DFL);
    kill(num, SIGQUIT);
    signal(SIGQUIT, SIG_IGN);
    find_pid_node_and_delete(num);
    return 0;
}

int builtin_jobs() // jobs内置命令，查看当前所有记录的进程信息
{
    delete_died_node();
    NODE *prout = head->next;
    while (prout != NULL)
    {
        if (prout->running)
            printf("%d, Running, %s\n", prout->npid, prout->backcn);
        else
            printf("%d, Stopped, %s\n", prout->npid, prout->backcn);
        prout = prout->next;
    }
    return 0;
}

int builtin_bg(char *arg) // bg内置命令，将任务转到后台运行
{
    int num = 0;
    int i = 0;
    int flag = 0;
    if (strlen(arg) == 0)
        return 0;
    for (i = 0; i < strlen(arg); i++)
        if (!isdigit(arg[i]))
        {
            printf("Can not bg pid: %s!\n", arg);
            return 0;
        }
    num = atoi(arg);
    //printf("I want to bg %d\n", num);
    NODE *tmp = head->next;
    while (tmp != NULL)
    {
        if (tmp->running == 0 && tmp->npid == num)
        {
            printf("bg pid %d success\n", num);
            tmp->running = 1;
            kill(num, SIGCONT);
            flag = 1;
            break;
        }
        tmp = tmp->next;
    }
    if (flag == 0)
        printf("Can not found a stopped pid %d!\n", num);
    return 0;
}

int builtin_fg(char *arg) // fg内置命令，将任务转到前台运行
{
    int num = 0;
    int i = 0;
    int flag = 0;
    if (strlen(arg) == 0)
        return 0;
    for (i = 0; i < strlen(arg); i++)
        if (!isdigit(arg[i]))
        {
            printf("Can not fg pid: %s!\n", arg);
            return 0;
        }
    num = atoi(arg);
    //printf("I want to fg %d\n", num);
    NODE *tmp = head->next;
    set_sig();
    while (tmp != NULL)
    {
        if (tmp->running == 0 && tmp->npid == num)
        {
            printf("fg pid %d success\n", num);
            tmp->running = 1;
            tmp->backgnd = 0;
            lastpid = num;
            kill(num, SIGCONT);
            flag = 1;
            break;
        }
        tmp = tmp->next;
    }
    if (flag == 0)
        printf("Can not found a stopped pid %d!\n", num);
    else
        waiting_for_pid(num);
    return 0;
}

int builtin_addpath(char *arg) // addpath内置命令，在path中增加输入的内容
{
    if (strlen(arg) <= 0)
        return 0;
    char now_path[3000];
    char tmp[2000];
    strcpy(tmp, getenv("PATH"));
    strcpy(now_path, "PATH=");
    strcat(now_path, arg);

    if (now_path[strlen(now_path) - 1] == '/')
        now_path[strlen(now_path) - 1] = '\0';
    strcat(now_path, ":");
    strcat(now_path, tmp);
    //printf("%s\n", now_path);
    putenv(now_path);
    //printf("after : %s\n", getenv("PATH"));
    return 0;
}

int builtin_envpath() // envpath内置命令，读取当前PATH环境变量
{
    printf("PATH=%s\n", getenv("PATH"));
    return 0;
}

int builtin_history() // history内置命令，读取历史记录
{
    int i = 0;
    HIST_ENTRY ** his;
    his = history_list();
    while(his[i] != NULL)
    {
        printf("%s\n", his[i]->line);
        i++;
    }
    return 0;
}

const int builtin_cmd_num = (sizeof(builtin_cmd_list) / sizeof(Builtin_cmd)) - 1;

char *Trim(char *str) // 剔除字符串首尾的空白字符(含空格)
{
    if (str == NULL)
        return "NULL";

    char *p_head = str;
    while (isspace(*p_head))
        p_head++;

    if (*p_head == '\0')
        return p_head;

    char *p_tail = p_head + strlen(p_head) - 1;
    while (p_tail > p_head && isspace(*p_tail))
        p_tail--;
    *(++p_tail) = '\0';

    return p_head;
}

Builtin_cmd *Find_builtin_cmd(char *name) // 在内置命令表中寻找对应指令
{
    register int i;
    for (i = 0; i < builtin_cmd_num; i++)
        if (strcmp (name, builtin_cmd_list[i].name) == 0)
        return (&builtin_cmd_list[i]);
    return ((Builtin_cmd *)NULL);
}

int Exec_cmd()//执行命令，未找到内置命令返回-1，否则返回内置命令返回值
{
    int i = 0;
    Builtin_cmd *command;
    char *word;
    char *line = line_trim;
    // Isolate the command word.
    while (line[i] && whitespace (line[i]))
        i++;
    word = line + i;

    while (line[i] && !whitespace (line[i]))
        i++;

    if (line[i])
        line[i++] = '\0';

    command = Find_builtin_cmd(word);

    if (!command) // 未找到对应内置命令
        return -1;
    while (whitespace (line[i]))
        i++;
    word = line + i;
    return (*(command->func))(word);
}

char cmd_prefix[512];
char *Get_name_and_dir() // 用户名和当前路径
{
    strcpy(cmd_prefix, "\033[0;32m");
    struct passwd *passwd_tmp = getpwuid(getuid());
    strcat(cmd_prefix, passwd_tmp->pw_name);
    strcat(cmd_prefix, "@");

    char name[128];
    gethostname(name, sizeof(name) - 1);
    strcat(cmd_prefix, name);
    strcat(cmd_prefix, ": ");

    char pwd[128];
    getcwd(pwd, sizeof(pwd) - 1);
    strcat(cmd_prefix, pwd);
    strcat(cmd_prefix, "$ ");
    strcat(cmd_prefix, "\033[0m");
    return cmd_prefix;
}

char *Read_cmd_line()
{
    //若已分配命令行缓冲区，则将其释放
    if (line_read)
    {
        free(line_read);
        line_read = NULL;
    }
    //读取用户输入的命令行
    
    line_read = readline(Get_name_and_dir());

    //剔除命令行首尾的空白字符。若剔除后的命令不为空，则存入历史列表
    line_trim = Trim(line_read);
    if (line_trim && *line_trim)
        add_history(line_trim);

    return line_trim;
}

char *Cmd_generator(char *text, int state)
{
    static int list_index, len;
    char *name;
    /* If this is a new word to complete, initialize now.  This includes
        saving the length of TEXT for efficiency, and initializing the index
        variable to 0. */
    if (!state)
    {
        list_index = 0;
        len = strlen(text);
    }
    // Return the next name which partially matches from the command list.
    for (; list_index < builtin_cmd_num; list_index++)
    {
        name = builtin_cmd_list[list_index].name;
        if (strncmp (name, text, len) == 0)
            return (strdup(name));
    }
    // If no names matched, then return NULL.
    return ((char *)NULL);
}

char **Cmd_complete(const char *text, int start, int end) // readline自动补全
{
    char **matches = NULL;
    if (start == 0)
        matches = rl_completion_matches(text, (rl_compentry_func_t *)Cmd_generator);
    return matches;
}

void Init_readline(void)//初始化Tab键能补齐的Cmd_complete函数
{
    rl_attempted_completion_function = Cmd_complete;
}

int recv_pause = 0;
char lastpid_info[1000];

void waiting_for_pid(int pid)
{
	recv_pause = 0;
	// 前台作业，需要等待管道中最后一个命令退出
	int status;
	delete_died_node();
	while (waitpid(pid, &status, WNOHANG) == 0)
		if (recv_pause)
			break;
		else
			usleep(1000);
}

void father_pause_proc()
{
    if (can_find_pid_node(lastpid)) // 若进程信息链表中有这个pid的任务，则给他发送stop
    {
	    recv_pause = 1;
        kill(lastpid, SIGSTOP);
        find_pid_node_and_change_backgnd(lastpid, 1);
        find_pid_node_and_change_running(lastpid, 0);
        printf("\npid: %d, [Stopped]\n", lastpid);
    }
    else
    {
        printf("\n%s", Get_name_and_dir());
    }
}

void add_job_into_list(int pid, int backgnd, int running, char *cmd)
{
	// 添加入jobs的链表
	// printf("Add this job into list\n");
	NODE *p = (NODE *)malloc(sizeof(NODE));
	p->npid = pid;
	p->backgnd = backgnd;
	p->running = running;
	strcpy(p->backcn, cmd);
	// printf("%s",p->backcn);
	NODE *tmp = head->next;
	head->next = p;
	p->next = tmp;
}

void forkexec(int i)
{
	pid_t pid;
	pid = fork();
	if (pid == -1)
	{
		perror("Fork error\n");
		return;
	}

	if (pid > 0)
	{
		// 父进程
		add_job_into_list(pid, backgnd, 1, cmdline);
		lastpid = pid;
		strcpy(lastpid_info, cmdline);
		//usleep(800);
	}
	else if (pid == 0)
	{
        // 子进程
		// backgnd=1时，将第一条简单命令的infd重定向至/dev/null
		// 当第一条命令试图从标准输入获取数据的时候立即返回EOF

		if (cmd[i].infd == 0 && backgnd == 1)
			cmd[i].infd = open("/dev/null", O_RDONLY);

		if (cmd[i].infd != 0)
		{
			close(0);
			dup(cmd[i].infd);
		}
		if (cmd[i].outfd != 1)
		{
			close(1);
			dup(cmd[i].outfd);
		}

		int j;
		for (j = 3; j < OPEN_MAX; ++j)
			close(j);

		// 前台作业能够接收SIGINT(ctrl C), SIGTSTP(ctrl Z), SIGQUIT信号
		// 将信号要恢复为默认操作
		signal(SIGTSTP, SIG_IGN);
		if (backgnd == 0)
		{
			signal(SIGINT, SIG_DFL);
			signal(SIGQUIT, SIG_DFL);
		}
		// printf("child\n");
		execvp(cmd[i].args[0], cmd[i].args);
		exit(0);
	}
}

int execute_command(void) // 执行外部命令
{
	if (cmd_count == 0)
		return 0;

	if (infile[0] != '\0')
		cmd[0].infd = open(infile, O_RDONLY);

	if (outfile[0] != '\0')
	{
		if (append)
			cmd[cmd_count - 1].outfd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0666);
		else
			cmd[cmd_count - 1].outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	}

	int i;
	int fd;
	int fds[2];
	for (i = 0; i < cmd_count; ++i)
	{
		// 如果不是最后一条命令，则需要创建管道
		if (i < cmd_count - 1)
		{
			pipe(fds);
			cmd[i].outfd = fds[1];
			cmd[i + 1].infd = fds[0];
		}

		forkexec(i);

		if ((fd = cmd[i].infd) != 0)
			close(fd);

		if ((fd = cmd[i].outfd) != 1)
			close(fd);
	}

	if (backgnd == 0)
	{
		waiting_for_pid(lastpid);
	}

	return 0;
}
