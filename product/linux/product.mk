# $@ 目标文件
# $^ 全部依赖文件
# $< 第一个依赖文件

INC+= -I$(ROOTDIR)/product/linux 

product: lib $(TARGET)/$(BIN)

TAGET_FILES := $(ROOTDIR)/product/linux/main.c \
$(ROOTDIR)/product/linux/x86_drv.c \
$(ROOTDIR)/platform/gmcu/core_gagent.c \
$(ROOTDIR)/iof/iof_hook.c

#target file
$(TARGET)/$(BIN):$(OBJ)/$(LIBTARGET)
	$(CC) $(CFLAGS) $(TAGET_FILES) $(INC) -o $(TARGET)/$(BIN) -l$(BIN)_$(OS)$(DEBUG) -L ./$(OBJ)
