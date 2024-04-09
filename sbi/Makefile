SRC_C = $(wildcard *.c)
SRC_S = $(wildcard *.S)

SBI_BUILD_DIR = ../$(BUILD_DIR)/sbi
UTILS_BUILD_DIR = ../$(BUILD_DIR)/utils

OBJS = $(patsubst %.S, $(SBI_BUILD_DIR)/%.o ,$(SRC_S) )
OBJS += $(patsubst %.c, $(SBI_BUILD_DIR)/%.o ,$(SRC_C) )


$(SBI_BUILD_DIR)/sbi: $(OBJS) sbi.ld $(SRC_C) $(SRC_S)
	$(LD) $(LDFLAGS) -T sbi.ld -o $(SBI_BUILD_DIR)/sbi-elf $(OBJS)
	./$(UTILS_BUILD_DIR)/capture_elf $(SBI_BUILD_DIR)/sbi-elf -o $(SBI_BUILD_DIR)/sbi


$(SBI_BUILD_DIR)/%.o : %.S
	@echo $@ $^
	$(CC) -M $(CFLAGS) -o $(patsubst %.o, %.d ,$@) $^   
	$(CC) -c $(CFLAGS) -o $@ $^

$(SBI_BUILD_DIR)/%.o : %.c
	@echo $@ $^
	$(CC) -M $(CFLAGS) -o $(patsubst %.o, %.d ,$@) $^ 
	$(CC) -c $(CFLAGS) -o $@ $^

all : 
	@if [ ! -d  "$(SBI_BUILD_DIR)" ]; \
		then mkdir $(SBI_BUILD_DIR); \
		fi;
	make $(SBI_BUILD_DIR)/sbi
	
