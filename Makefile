TARGET = libequeue.a
CC = gcc
AR = ar

SRC += equeue_no_os.c \
equeue.c 

OBJ := $(SRC:.c=.o)
DEP := $(SRC:.c=.d)
ASM := $(SRC:.c=.s)

ifdef DEBUG
CFLAGS += -O0 -g3
else
CFLAGS += -O2
endif

CFLAGS += -I.
CFLAGS += -std=c99
CFLAGS += -Wall

LFLAGS += -pthread

all: $(TARGET)

%.a: $(OBJ)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) -c -MMD $(CFLAGS) $< -o $@

%.s: %.c
	$(CC) -S $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGET)
	rm -f $(OBJ)
	rm -f $(DEP)
	rm -f $(ASM)