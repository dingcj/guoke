include ../tool.mk

HTTP_INTERFACE:=$(PWD)
HTTP_INTERFACE_SUBDIRS := $(shell find $(HTTP_INTERFACE) -type d)
HTTP_INTERFACE_SRC := $(foreach dir,$(HTTP_INTERFACE_SUBDIRS),$(wildcard $(dir)/*.c))
HTTP_INTERFACE_OBJ := $(patsubst %.c,%.o, $(HTTP_INTERFACE_SRC))

LIB_CURL_DIR := ../libcurl

CFLAGS=-I$(HTTP_INTERFACE) -I$(LIB_CURL_DIR)/include

TARGET=libhttp_interface.a
all:$(TARGET)
$(TARGET): $(HTTP_INTERFACE_OBJ)
	$(AR) crv $(TARGET) $(HTTP_INTERFACE_OBJ)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	rm -f $(HTTP_INTERFACE_OBJ)
	rm -f $(TARGET)
