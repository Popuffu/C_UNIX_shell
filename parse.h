#ifndef _PARSE_H_
#define _PARSE_H_

void set_sig(void);
void init(void);
void read_command(void);
int parse_command(void);
int check_and_move(const char *str);
void get_command(int i);
void getname(char *name);
void print_command();

#endif
