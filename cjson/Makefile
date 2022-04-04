include ../tool.mk

CJSON:=$(PWD)
CJSON_SUBDIRS := $(shell find $(CJSON) -type d)
CJSON_SRC := $(foreach dir,$(CJSON_SUBDIRS),$(wildcard $(dir)/*.c))
CJSON_OBJ := $(patsubst %.c,%.o, $(CJSON_SRC))

CFLAGS=-I$(CJSON)

TARGET=libcjson.a
all:$(TARGET)
$(TARGET): $(CJSON_OBJ)
	$(AR) crv $(TARGET) $(CJSON_OBJ)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	rm -f $(CJSON_OBJ)
	rm -f $(TARGET)
