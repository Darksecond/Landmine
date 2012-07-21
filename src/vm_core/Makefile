CC=gcc
LIBTOOL=libtool

SRC=vm.c
OBJ=$(SRC:.c=.o)
OUT=libvm_core.a
CFLAGS=-Wall

all: $(OUT)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	$(LIBTOOL) -o $(OUT) -static $(OBJ)
