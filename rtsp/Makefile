include ../tool.mk

PORTING_ROOT_DIR ?= $(shell cd .. && /bin/pwd)
include $(PORTING_ROOT_DIR)/porting_base.mk

RTSP := $(PWD)
RTSP_SUBDIRS := $(shell find $(RTSP) -type d)
RTSP_SRC := $(foreach dir,$(RTSP_SUBDIRS),$(wildcard $(dir)/*.c))
RTSP_OBJ := $(patsubst %.c,%.o, $(RTSP_SRC))

SAMPLE_COMMON_DIR := ../sample/common

CFLAGS=-I$(RTSP) -I$(SAMPLE_COMMON_DIR) -I../include -I$(GMP_DIR)/include

TARGET=librtsp.a
all:$(TARGET)
$(TARGET): $(RTSP_OBJ)
	$(AR) crv $(TARGET) $(RTSP_OBJ)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	rm -f $(RTSP_OBJ)
	rm -f $(TARGET)
