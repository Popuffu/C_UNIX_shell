#include "parse.h"
#include "def.h"
#include "builtin.h"

char cmdline[MAXLINE+1];
char avline[MAXLINE+1];
char *lineptr;
char *avptr;

char infile[MAXNAME+1];
char outfile[MAXNAME+1];
COMMAND cmd[PIPELINE];

int cmd_count;
int backgnd;
int append;
int lastpid;

NODE *head;

int main(void)
{
    head=(NODE*)malloc(sizeof(NODE));
    head->next=NULL;
	Init_readline();
	while (1)
	{
		init(); // 初始化
		set_sig(); // 更改信号量处理方式
		read_command(); // 读取输入
		if (Exec_cmd() < 0) // 是否为内置命令
		{
			// 不是内置命令
			parse_command(); // 分割
			execute_command(); // 执行
		}
	}
	printf("\nexit\n");
	return 0;
}
