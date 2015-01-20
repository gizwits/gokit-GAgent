# $@ 目标文件
# $^ 全部依赖文件
# $< 第一个依赖文件
INC+= -I $(ROOTDIR)/software/application/ -I $(ROOTDIR)/software/cloud/ -I $(ROOTDIR)/software/lib/ -I $(ROOTDIR)/software/lan/ -I $(ROOTDIR)/iof/ \
      -I $(ROOTDIR)/software/
lib: $(OBJ)/$(LIBTARGET)


$(OBJ)/$(LIBTARGET): \
	$(OBJ)/Wifi_mode.o \
	$(OBJ)/xpgmain.o \
	$(OBJ)/mqttlib.o \
	$(OBJ)/lan.o \
	$(OBJ)/mcu_v4.o \
	$(OBJ)/mcu_common.o \
	$(OBJ)/base64.o \
	$(OBJ)/Socket.o \
	$(OBJ)/MqttSTM.o \
	$(OBJ)/utilcoding.o \
	$(OBJ)/user_misc.o \
	$(OBJ)/mqttxpg.o \
	$(OBJ)/http.o \
	$(OBJ)/adapter_linux.o \
	$(OBJ)/gagent_login_cloud.o	\
	$(OBJ)/md5.o	\
    #ar rc $@ $^ #create static lib
	@$(AR) ru $(OBJ)/$(LIBTARGET) $^
	@$(AR) ru $(OBJ)/../lib/$(LIBTARGET) $^
# object files


$(OBJ)/Wifi_mode.o:$(SRC)/application/Wifi_mode.c $(SRC)/application/Wifi_mode.h
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<

$(OBJ)/xpgmain.o:$(SRC)/application/xpgmain.c $(SRC)/application/main.h
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<

$(OBJ)/http.o:$(SRC)/cloud/http.c $(SRC)/cloud/http.h
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<

$(OBJ)/mqttlib.o:$(SRC)/cloud/mqttlib.c $(SRC)/cloud/mqttlib.h
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<

$(OBJ)/MqttSTM.o:$(SRC)/cloud/MqttSTM.c $(SRC)/cloud/MqttSTM.h
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<

$(OBJ)/mqttxpg.o:$(SRC)/cloud/mqttxpg.c $(SRC)/cloud/mqttxpg.h
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<

$(OBJ)/lan.o:$(SRC)/lan/lan.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<

$(OBJ)/mcu_v4.o:$(SRC)/lan/mcu_v4.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<
	
$(OBJ)/mcu_common.o:$(SRC)/lan/mcu_common.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<

$(OBJ)/Socket.o:$(SRC)/lan/Socket.c $(SRC)/lan/Socket.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<

$(OBJ)/base64.o:$(SRC)/lib/base64.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<

$(OBJ)/user_misc.o:$(SRC)/lib/user_misc.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<

$(OBJ)/utilcoding.o:$(SRC)/lib/utilcoding.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<

$(OBJ)/adapter_linux.o:$(SRC)/lib/adapter_linux.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<
	
$(OBJ)/gagent_login_cloud.o:$(SRC)/gagent_login_cloud.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<
	
$(OBJ)/md5.o:$(SRC)/lib/md5.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<