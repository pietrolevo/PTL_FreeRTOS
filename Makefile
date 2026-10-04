# General utils
CC = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
GDB = gdb-multiarch
BUILD_DIR = build

CFLAGS = -mcpu=cortex-m3 -mthumb -O0 -g -Wall \
-I./include \
-I./FreeRTOS/Source/include \
-I./FreeRTOS/Source/portable/GCC/ARM_CM3 \
-I./src \
-I./tests \
-I./tests/include
CFLAGS += -DPOLICY_$(POLICY)

LDFLAGS = -T src/linker.ld -nostartfiles --specs=nosys.specs
SRCS = \
src/startup.c \
src/ptl.c \
src/uart.c \
FreeRTOS/Source/tasks.c \
FreeRTOS/Source/queue.c \
FreeRTOS/Source/list.c \
FreeRTOS/Source/timers.c \
FreeRTOS/Source/event_groups.c \
FreeRTOS/Source/portable/GCC/ARM_CM3/port.c \
FreeRTOS/Source/portable/MemMang/heap_4.c 

# PTL entries

POLICY ?= SKIP
TEST_UTILS = tests/src/test_utils.c
TEST = $(wildcard tests/*.c)
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
TEST_UTILS_OBJ = $(TEST_UTILS:%.c=$(BUILD_DIR)/%.o)
OBJST = $(TEST:%.c=$(BUILD_DIR)/%.o)
ELFS = $(TEST:tests/%.c=$(BUILD_DIR)/tests/%.elf)

VERBOSE ?= 1

ifeq ($(VERBOSE), 1)
    VERBOSE_FLAG := -verbose
else
    VERBOSE_FLAG :=
endif


all: directories $(ELFS)

directories:
	@mkdir -p $(BUILD_DIR)/src
	@mkdir -p $(BUILD_DIR)/tests
	@mkdir -p $(BUILD_DIR)/FreeRTOS/Source/portable/GCC/ARM_CM3
	@mkdir -p $(BUILD_DIR)/FreeRTOS/Source/portable/MemMang

$(BUILD_DIR)/tests/%.elf: $(BUILD_DIR)/tests/%.o $(OBJS) $(TEST_UTILS_OBJ)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(TEST_UTILS_OBJ) $< $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

#
# -- TEST -- 
#

VENV_DIR := tests/config/venv
PYTHON := $(VENV_DIR)/bin/python3

$(PYTHON):
	python3 -m venv $(VENV_DIR)
	$(PYTHON) -m pip install -r tests/config/requirements.txt

run_tests: $(ELFS) $(PYTHON)
	@for test in $(ELFS); do \
		testname=$$(basename $$test .elf) ; \
		qemu-system-arm \
			-machine mps2-an385 \
			-cpu cortex-m3 \
			-kernel $$test \
			-nographic \
			-icount shift=1,align=off,sleep=off \
			-semihosting \
			-serial mon:stdio | $(PYTHON) tests/tools/checker.py $$testname $(VERBOSE_FLAG) ; \
		sleep 1 ; \
	done
	@cat tests/logs/*_report.rpt > tests/logs/summary.rpt


run-%: $(BUILD_DIR)/tests/%.elf $(PYTHON)
	@mkdir -p tests/logs
	qemu-system-arm \
		-machine mps2-an385 \
		-cpu cortex-m3 \
		-kernel $< \
		-nographic \
		-icount shift=1,align=off,sleep=off \
		-semihosting \
		-serial mon:stdio | $(PYTHON) tests/tools/checker.py $* $(VERBOSE_FLAG)


run_print-%: $(BUILD_DIR)/tests/%.elf $(PYTHON)
	@mkdir -p tests/logs
	qemu-system-arm \
		-machine mps2-an385 \
		-cpu cortex-m3 \
		-kernel $< \
		-nographic \
		-semihosting \
		-icount shift=1,align=off,sleep=off \
		-serial mon:stdio


#
# -- config framework --
#

FILE ?= taskset1
analyze: $(PYTHON)
	$(PYTHON) config_framework.py analyze $(FILE).yaml

generate: $(PYTHON)
	$(PYTHON) config_framework.py generate $(FILE).yaml

#
# -- DOCUMENTATION -- 
#

DOCS_DIR = docs/html

doc-gen:
	doxygen Doxyfile

ifeq ($(shell uname), Darwin)
    BROWSER ?= open
else
    BROWSER ?= xdg-open
endif


doc-open: doc-gen
	@$(BROWSER) $(DOCS_DIR)/index.html

clean:
	@rm -f $(OBJS) $(OBJST) $(ELFS)
	@rm -rf $(BUILD_DIR) tracing/*
	@rm -rf $(DOCS_DIR)/html/
	@rm -rf tests/logs/*
	@rm -rf tests/tools/__pycache__


clean_all: clean
	@rm -rf $(VENV_DIR)

.PHONY: all directories setup run_tests clean clean_all analyze generate doc-gen doc-open
