SHELL=/bin/bash

CC = gcc
LINK = gcc

CFLAGS = -std=c99 -g -I./include -Isrc
LFLAGS = -lprosopon -lprosopon-interpreter

SRC_DIR = src

OBJS = main.o pro_alloc.o

OUT_DIR = build
OUT_OBJS = $(addprefix $(OUT_DIR)/,$(OBJS))


all : $(OUT_OBJS)
	$(LINK) $(LFLAGS) -o prosopon $^

$(OUT_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@


.PHONY : clean
clean :
	rm -f $(OUT_DIR)/*
	if [ -f prosopon ]; then rm prosopon; fi

