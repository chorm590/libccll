#
# A common c-language library in linux
# Started at 2025-10-31

VERSION := 0.0.0
NAME := ccll
OUTPUT_NAME := lib$(NAME).so

CROSS_COMPILE :=
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
AR := $(CROSS_COMPILE)ar

SRCS := main.c \
		sys/sh.c

OBJS := $(SRCS:.c=.o)
CFLAGS := -fPIC -shared -Wall
INCS := -Icomm/inc \
		-Isys/inc \
		-Icfg/inc \
		-Ilog/inc

OBJ_DIR := out/obj
OBJS_C := $(addprefix $(OBJ_DIR)/, $(OBJS))

all: clean env ext_lib $(OBJS)
	@echo "making $(NAME)..."
	$(CC) $(CFLAGS) -o out/lib/$(OUTPUT_NAME) $(OBJS_C)
	@echo -e "\e[32mmake done\e[0m"

env:
	@echo "making enviroment"
	@if [ ! -d out ]; then mkdir out; fi
	@if [ ! -d out/include ]; then mkdir out/include; fi
	@if [ ! -d out/lib ]; then mkdir out/lib; fi
	@if [ ! -d out/etc ]; then mkdir out/etc; fi

ext_lib:
	@echo "making external library"

%.o: %.c
	@if [ ! -d $(OBJ_DIR)/$$(dirname $@) ]; then mkdir -p $(OBJ_DIR)/$$(dirname $@); fi
	$(CC) -c $< $(INCS) -o $(OBJ_DIR)/$@

cfgs:
	@mkdir -p out/etc/
	@cp etc/*.ini.prod out/etc/ || exit 1

clean:
	@echo "cleaning..."
	@if [ -d out ]; then rm -rf out; fi

test:
	@echo "making testing..."


.PHONY: all clean ext_lib test

