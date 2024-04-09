
UTILS_BUILD_DIR = ../$(BUILD_DIR)/utils
$(UTILS_BUILD_DIR)/capture_elf: capture_elf.c
	gcc capture_elf.c -o $(UTILS_BUILD_DIR)/capture_elf
.PHONY: all_utils all
all_utils : $(UTILS_BUILD_DIR)/capture_elf

all :
	@if [ ! -d  "$(UTILS_BUILD_DIR)" ]; \
		then mkdir $(UTILS_BUILD_DIR); \
		fi;
	make all_utils