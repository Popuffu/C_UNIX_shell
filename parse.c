#include "parse.h"
#include "externs.h"
#include "builtin.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include <string.h>
#include <fcntl.h>

void set_sig(void)
{
	signal(SIGINT, SIG_IGN); // 忽略
	signal(SIGTSTP, father_pause_proc); // ctrl + Z
	signal(SIGQUIT, SIG_IGN); // 忽略
}

void init(void)
{
	memset(cmd, 0, sizeof(cmd));
	int i;
	for (i=0; i<PIPELINE; ++i)
	{
		cmd[i].infd = 0;
		cmd[i].outfd = 1;
	}
	memset(cmdline, 0, sizeof(cmdline));
	memset(avline, 0, sizeof(avline));
	lineptr = cmdline;
	avptr = avline;
	memset(infile, 0, sizeof(infile));
	memset(outfile, 0, sizeof(outfile));
	cmd_count = 0;
	backgnd = 0;
	append = 0;
	lastpid = 0;
    
	fflush(stdout);
}

void read_command(void)
{
	// 按行读取命令，cmdline中包含\n
	char *tmp_line = Read_cmd_line();
	strcpy(cmdline, tmp_line);
	strcat(cmdline, "\n");
}

//成功返回解析到的命令个数，失败返回-1
int parse_command(void)
{
	/* cat < test.txt | grep -n public > test2.txt & */
	if (check_and_move("\n"))
		return 0;

	// 1、解析第一条简单命令
       
	get_command(0);
	// 2、判定是否有输入重定向符 
	if (check_and_move("<"))
		getname(infile);
	// 3、判定是否有管道 
	int i;
	for (i=1; i < PIPELINE; ++i)
	{
		if (check_and_move("|"))
			get_command(i);
		else
			break;
	}
	// 4、判定是否有输出重定向符
	if (check_and_move(">"))
	{
		if (check_and_move(">")) // >>
			append = 1;
		getname(outfile);
	}
	// 5、判定是否后台作业 
	if (check_and_move("&"))
		backgnd = 1;
	// 6、判定命令结束‘\n’
	if (check_and_move("\n"))
	{
		cmd_count = i;
		return cmd_count;
	}
	else
	{
		fprintf(stderr, "Command line syntax error\n");
		return -1;
	}
}

void print_command()
{
	int i;
	int j;
	printf("cmd_count = %d\n", cmd_count);
	if (infile[0] != '\0')
		printf("infile=[%s]\n", infile);
	if (outfile[0] != '\0')
		printf("outfile=[%s]\n", outfile);

	for (i=0; i<cmd_count; ++i)
	{
		j = 0;
		while (cmd[i].args[j] != NULL)
		{
			printf("[%s] ", cmd[i].args[j]);
			j++;
		}
		printf("\n");
	}
}

/*
 * 解析简单命令至cmd[i]
 * 提取cmdline中的命令参数到avline数组中，并且将COMMAND结构中的args[]中的每个指针指向这些字符串
 */
void get_command(int i)
{
	// cat < test.txt | grep -n public > test2.txt &

	int j = 0;
	int inword;
	while (*lineptr != '\0')
	{
		// 去除空格
		while (*lineptr == ' ' || *lineptr == '\t')
			lineptr++;

		// 将第i条命令第j个参数指向avptr
		cmd[i].args[j] = avptr;
		// 提取参数
		while (*lineptr != '\0'
			&& *lineptr != ' '
			&& *lineptr != '\t'
			&& *lineptr != '>'
			&& *lineptr != '<'
			&& *lineptr != '|'
			&& *lineptr != '&'
			&& *lineptr != '\n')
		{
				// 参数提取至avptr指针所向的数组avline
				*avptr++ = *lineptr++;
				inword = 1;
		}
		*avptr++ = '\0';
		switch (*lineptr)
		{
		case ' ':
		case '\t':
			inword = 0;
			j++;
			break;
		case '<':
		case '>':
		case '|':
		case '&':
		case '\n':
			if (inword == 0)
				cmd[i].args[j] = NULL;
			return;
		default: /* for '\0' */
			return;
		}
	}
}

/*
 * 将lineptr中的字符串与str进行匹配
 * 成功返回1，lineptr移过所匹配的字符串
 * 失败返回0，lineptr保持不变
 */
int check_and_move(const char *str)
{
	char *p;
	while (*lineptr == ' ' || *lineptr == '\t')
		lineptr++;

	p = lineptr;
	while (*str != '\0' && *str == *p)
	{
		str++;
		p++;
	}
	if (*str == '\0')
	{
		lineptr = p; // lineptr移过所匹配的字符串
		return 1;
	}
	// lineptr保持不变
	return 0;
}

void getname(char *name)
{
	while (*lineptr == ' ' || *lineptr == '\t')
		lineptr++;

	while (*lineptr != '\0' && *lineptr != ' ' && *lineptr != '\t' && *lineptr != '>' 
	&& *lineptr != '<' && *lineptr != '|' && *lineptr != '&' && *lineptr != '\n')
		*name++ = *lineptr++;
	*name = '\0';
}
