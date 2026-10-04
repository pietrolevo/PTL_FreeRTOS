CC = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
BUILD_DIR = build

CFLAGS = -mcpu=cortex-m3 -mthumb -O0 -g -Wall \
-I./include \
-I../include \
-I../FreeRTOS/Source/include \
-I../FreeRTOS/Source/portable/GCC/ARM_CM3

LDFLAGS = -T src/linker.ld -nostartfiles --specs=nosys.specs

SRCS = \
../src/ptl.c \
../FreeRTOS/Source/tasks.c \
../FreeRTOS/Source/queue.c \
../FreeRTOS/Source/list.c \
../FreeRTOS/Source/timers.c \
../FreeRTOS/Source/event_groups.c \
../FreeRTOS/Source/portable/GCC/ARM_CM3/port.c \
../FreeRTOS/Source/portable/MemMang/heap_4.c \
src/startup.c \
src/uart.c \
src/demo_tasks.c \
src/main.c

OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(subst ../,parent_,$(SRCS)))
ELF  = $(BUILD_DIR)/demo.elf

all: $(ELF)

$(ELF): $(OBJS)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(BUILD_DIR)/parent_%.o: ../%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(ELF)
	qemu-system-arm \
		-machine mps2-an385 \
		-cpu cortex-m3 \
		-kernel build/demo.elf \
		-nographic \
		-semihosting \
		-serial mon:stdio

clean:
	@rm -rf $(BUILD_DIR)

.PHONY: all run clean