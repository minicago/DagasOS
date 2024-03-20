SRC_C = $(wildcard *.c)
SRC_S = $(wildcard *.S)

U_LIB_BUILD_DIR = ../../$(BUILD_DIR)/user/lib

OBJS = $(patsubst %.S, $(U_LIB_BUILD_DIR)/%.o ,$(SRC_S) )
OBJS += $(patsubst %.c, $(U_LIB_BUILD_DIR)/%.o ,$(SRC_C) )

CFLAGS += -I../include

$(U_LIB_BUILD_DIR)/%.o : %.S
	@echo $@ $^
	$(CC) -M $(CFLAGS) -o $(patsubst %.o, %.d ,$@) $^   
	$(CC) -c $(CFLAGS) -o $@ $^

$(U_LIB_BUILD_DIR)/%.o : %.c
	@echo $@ $^
	$(CC) -M $(CFLAGS) -o $(patsubst %.o, %.d ,$@) $^ 
	$(CC) -c $(CFLAGS) -o $@ $^

.PHONY:all_lib
all_lib : $(OBJS)

all : 
	@if [ ! -d  "$(U_LIB_BUILD_DIR)" ]; \
		then mkdir $(U_LIB_BUILD_DIR); \
		fi;
	make all_lib