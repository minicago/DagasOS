SRC_C = $(wildcard *.c)
SRC_S = $(wildcard *.S)

LIB_SRC_C = $(wildcard lib/*.c)
LIB_SRC_S = $(wildcard lib/*.S)

U_BUILD_DIR = ../$(BUILD_DIR)/user

OBJS = $(patsubst %.S, $(U_BUILD_DIR)/%.o ,$(SRC_S) )
OBJS += $(patsubst %.c, $(U_BUILD_DIR)/%.o ,$(SRC_C) )

UPROGS = $(patsubst %.S, $(U_BUILD_DIR)/_% ,$(SRC_S) )
UPROGS += $(patsubst %.c, $(U_BUILD_DIR)/_% ,$(SRC_C) )

$(U_BUILD_DIR)/%.o : %.S
	@echo $@ $^
	$(CC) -M $(CFLAGS) -march=rv64imac -mabi=lp64 -o $(patsubst %.o, %.d ,$@) $^   
	$(CC) -c $(CFLAGS) -march=rv64imac -mabi=lp64 -o $@ $^

$(U_BUILD_DIR)/%.o : %.c
	@echo $@ $^
	$(CC) -M $(CFLAGS) -march=rv64imac -mabi=lp64 -o $(patsubst %.o, %.d ,$@) $^ 
	$(CC) -c $(CFLAGS) -march=rv64imac -mabi=lp64 -o $@ $^

.PHONY:all_prog

$(U_BUILD_DIR)/_%: $(U_BUILD_DIR)/%.o $(ULIBS)
	@echo $(ULIBS)
	$(LD) $(LDFLAGS) -T lib/arch/riscv/user.ld -Ttext 0x1000 -o $@ $^ build/libulib.a

all_prog : $(UPROGS)

# O0=>O3
all : 
	@if [ ! -d  "$(U_BUILD_DIR)" ]; \
		then mkdir $(U_BUILD_DIR); \
		fi;
	make all_prog