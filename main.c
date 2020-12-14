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
		/* 初始化环境 */
		init();
		set_sig();
		/* 获取命令 */
		read_command();
		if (Exec_cmd() < 0)
		{
			/* 解析命令 */
			parse_command();
			//print_command();
			/* 执行命令 */
			execute_command();
		}
	}
	printf("\nexit\n");
	return 0;
}
