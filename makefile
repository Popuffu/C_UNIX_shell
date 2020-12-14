CC ?= clang
CFLAGS = -Wall -DDEBUG -g3
BIN = shell
OBJS = main.o parse.o builtin.o
READLINE = -lreadline
$(BIN):$(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(READLINE)
%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(READLINE)
clean:
	rm *.o