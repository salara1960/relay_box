NAME=relay

CFLAG =-std=gnu99 -Os -Wall -pipe -D_GNU_SOURCE

MACHINE := $(shell uname -m)
ifeq ($(MACHINE), x86_64)
	LIB_DIR = /usr/lib64
else
	LIB_DIR = /usr/lib
endif

$(NAME): rs232.o
	gcc -o $(NAME) rs232.o -L$(LIB_DIR) -lm -ldl
	strip $(NAME)
rs232.o: rs232.c
	gcc -c rs232.c $(CFLAG)
clean:
	rm -f *.o $(NAME)


