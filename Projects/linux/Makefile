CFLAGS=
PRODUCT ?= x86
VERSION_TYPE ?= DEBUG

ifeq ($(PRODUCT),x86)
	CC=gcc
	CPP=g++
	FLAGS=-m32
	CFLAGS+= -D GAGENT_PRODUCT_X86
	PLATFORM=x86
endif

ifeq ($(PRODUCT),mips32)
	CC=mipsel-linux-gcc
	CPP=mipsel-linux-g++
	FLAGS=-m32
	CFLAGS+= -DGAGENT_PRODUCT_MIPS32
	PLATFORM=mips32
endif

ifeq ($(PRODUCT),junzheng)
	CC=mipsel-linux-gcc
	CPP=mipsel-linux-g++
	FLAGS=-m32
	CFLAGS+= -DGAGENT_PRODUCT_JUNZHENG
	PLATFORM=mips32
endif

ifeq ($(VERSION_TYPE),DEBUG)
	OBJ=obj/debug
	DEBUG=debug
	CFLAGS+= -DGAGENT_RELEASE=0
endif

ifeq ($(VERSION_TYPE),RELEASE)
	OBJ=obj/release
	DEBUG=release
	CFLAGS+= -DGAGENT_RELEASE=1
endif

CFLAGS+= -DGAGENT_DEMO

OUTPUT=bin
BIN=gagent_$(PLATFORM)_$(DEBUG)

SRC=../../software
TARGET=target
ROOTDIR=../..
INS=/usr/local/lib
INC=-I$(SRC) -I$(ROOTDIR)/include -I. -I$(ROOTDIR) -I$(ROOTDIR)/software


OS?=linux

CFLAGS+=-g
LIBTARGET=lib$(BIN)_$(OS)$(DEBUG).a
# $@ 目标文件
# $^ 全部依赖文件
# $< 第一个依赖文件
.PHONY: lib product
all: $(PLATFORM)

folders:
	mkdir -p obj
	mkdir -p obj/lib
	mkdir -p $(OBJ)
	mkdir -p $(TARGET)

x86: folders \
	lib \
	product

mips32: folders \
	lib \
	product

TEST_DIR = $(ROOTDIR)/test
TEST_TAGET_FILES = $(TEST_DIR)/serial.c $(ROOTDIR)/product/linux/x86_drv.c $(ROOTDIR)/platform/gmcu/core_gagent.c $(ROOTDIR)/iof/iof_hook.c
TEST_BINS = $(TEST_DIR)/serial
test:x86
	$(CC) $(CFLAGS) $(TEST_TAGET_FILES) $(INC) -o $(TEST_BINS) -l$(BIN)_$(OS)$(DEBUG) -L ./$(OBJ)
	
include $(ROOTDIR)/software/lib.mk
include $(ROOTDIR)/product/linux/product.mk

clean:
	rm -rf obj
	rm -rf target
	rm -rf $(TEST_BINS)
