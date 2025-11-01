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

SRCS := comm/alloc.c \
		log/log.c \
		main.c

OBJS := $(SRCS:.c=.o)
CFLAGS_C := -fPIC -Wall
CFLAGS_L := -shared -Wall
INCS := -Icomm/inc \
		-Icfg/inc \
		-Ilog/inc \
		-Iqueue/inc \
		-Isys/inc \
		-Iinc

OBJ_DIR := out/obj
OBJS_C := $(addprefix $(OBJ_DIR)/, $(OBJS))

all: clean env ext_lib $(OBJS)
	@echo "making $(NAME)..."
	$(CC) $(CFLAGS_L) -o out/lib/$(OUTPUT_NAME) $(OBJS_C)
	@echo "installing the includes..."
	@for hdr in $$(find . -type f ! -name "_*" | grep "\.h$$"); \
		do \
			cp $$hdr out/include/cl_$$(basename $$hdr); \
		done
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
	$(CC) $(CFLAGS_C) -c $< $(INCS) -o $(OBJ_DIR)/$@

cfgs:
	@mkdir -p out/etc/
	@cp etc/*.ini.prod out/etc/ || exit 1

clean:
	@echo "cleaning..."
	@if [ -d out ]; then rm -rf out; fi

test: all
	@echo "making testing..."
	@if [ ! -d out/test ]; then mkdir out/test; fi
	$(CC) -o out/test/test test/main.c -Iout/include -Lout/lib -lccll
	@echo "test make done"

runtest:
	@if [ ! -f out/test/test ]; then echo "No test ready"; fi
	@echo "running test..."
	@LD_LIBRARY_PATH=out/lib ./out/test/test


.PHONY: all clean ext_lib test

