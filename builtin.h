#ifndef _BUILTIN_H_
#define _BUILTIN_H_

void Init_readline(void);
char *Trim(char *str);
int Exec_cmd();
char *Read_cmd_line();
void Print_name_and_dir();
int can_find_pid_node(int pid);
void find_pid_node_and_delete(int pid);
void find_pid_node_and_change_running(int pid, int running);
void find_pid_node_and_change_backgnd(int pid, int backgnd);
void delete_died_node();
int execute_command(void);
void father_pause_proc();
void waiting_for_pid(int pid);

#endif
