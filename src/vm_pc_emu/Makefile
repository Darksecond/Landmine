CC=gcc
SRC=main.c
OBJ=$(SRC:.c=.o)
OUT=pc_emu
CFLAGS=-Wall
LIB=../vm_core/libvm_core.a

all: $(OUT)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIB) -o $(OUT)
