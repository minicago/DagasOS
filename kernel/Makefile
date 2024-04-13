SRC_C = $(wildcard *.c)
SRC_S = $(wildcard *.S)

K_BUILD_DIR = ../$(BUILD_DIR)/kernel

OBJS = $(patsubst %.S, $(K_BUILD_DIR)/%.o ,$(SRC_S) )
OBJS += $(patsubst %.c, $(K_BUILD_DIR)/%.o ,$(SRC_C) )

$(K_BUILD_DIR)/kernel: $(OBJS) kernel.ld $(SRC_C) $(SRC_S) print
	$(LD) $(LDFLAGS) -T kernel.ld -o $(K_BUILD_DIR)/kernel $(OBJS)

print:
	$(CC) -c $(CFLAGS) $(DDEBUG) -o $(K_BUILD_DIR)/print.o print.c
$(K_BUILD_DIR)/%.o : %.S
	@echo $@ $^
	$(CC) -M $(CFLAGS) -o $(patsubst %.o, %.d ,$@) $^   
	$(CC) -c $(CFLAGS) -o $@ $^

$(K_BUILD_DIR)/%.o : %.c
	@echo $@ $^
	$(CC) -M $(CFLAGS) -o $(patsubst %.o, %.d ,$@) $^ 
	$(CC) -c $(CFLAGS) -o $@ $^

all : 
	@if [ ! -d  "$(K_BUILD_DIR)" ]; \
		then mkdir $(K_BUILD_DIR); \
		fi;
	make $(K_BUILD_DIR)/kernel
	
