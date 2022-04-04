PORTING_ROOT_DIR ?= $(shell cd $(CURDIR)/../.. && /bin/pwd)

include $(PORTING_ROOT_DIR)/porting_base.mk

TARGET := sample_venc

CFLAGS := $(PORTING_CFLAGS) $(PORTING_LIBS)

SRCS := $(wildcard ./*.c)

OBJS := $(patsubst %.c, %.o, $(SRCS))

.PHONY: all clean

all: $(OBJS)
	$(AT)$(CC) -o $(TARGET) $^ $(CFLAGS)

%.o : %.c
	$(AT)$(CC) -c -o $@ $< $(CFLAGS)

clean:
	$(AT)rm -rf $(OBJS) $(TARGET)

