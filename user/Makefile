SRC_C = $(wildcard *.c)
SRC_S = $(wildcard *.S)

LIB_SRC_C = $(wildcard lib/*.c)
LIB_SRC_S = $(wildcard lib/*.S)

U_BUILD_DIR = ../$(BUILD_DIR)/user

OBJS = $(patsubst %.S, $(U_BUILD_DIR)/%.o ,$(SRC_S) )
OBJS += $(patsubst %.c, $(U_BUILD_DIR)/%.o ,$(SRC_C) )

ULIBS = $(patsubst lib/%.S, $(U_BUILD_DIR)/lib/%.o ,$(LIB_SRC_S) )
ULIBS += $(patsubst lib/%.c, $(U_BUILD_DIR)/lib/%.o ,$(LIB_SRC_C) )

UPROGS = $(patsubst %.S, $(U_BUILD_DIR)/_% ,$(SRC_S) )
UPROGS += $(patsubst %.c, $(U_BUILD_DIR)/_% ,$(SRC_C) )

$(U_BUILD_DIR)/%.o : %.S
	@echo $@ $^
	$(CC) -M $(CFLAGS) -o $(patsubst %.o, %.d ,$@) $^   
	$(CC) -c $(CFLAGS) -o $@ $^

$(U_BUILD_DIR)/%.o : %.c
	@echo $@ $^
	$(CC) -M $(CFLAGS) -o $(patsubst %.o, %.d ,$@) $^ 
	$(CC) -c $(CFLAGS) -o $@ $^

.PHONY:all_prog

$(U_BUILD_DIR)/_%: $(U_BUILD_DIR)/%.o $(ULIBS)
	@echo $(ULIBS)
	$(LD) $(LDFLAGS) -T user.ld -o $@ $^

all_prog : $(UPROGS)

all : 
	@if [ ! -d  "$(U_BUILD_DIR)" ]; \
		then mkdir $(U_BUILD_DIR); \
		fi;
	make -C lib all
	make all_prog