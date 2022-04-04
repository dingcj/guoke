include ../tool.mk

SEARCH:=$(PWD)
SEARCH_SUBDIRS := $(shell find $(SEARCH) -type d)
SEARCH_SRC := $(foreach dir,$(SEARCH_SUBDIRS),$(wildcard $(dir)/*.c))
SEARCH_OBJ := $(patsubst %.c,%.o, $(SEARCH_SRC))

TARGET=search
CJSON=../cjson
HTTP_INTERFACE_DIR=../HttpInterface
CFLAGS=-I$(CJSON) -I$(HTTP_INTERFACE_DIR)

all:$(TARGET)
$(TARGET): $(SEARCH_OBJ)
	@$(CC) $(SEARCH_OBJ) -L$(CJSON) -L$(HTTP_INTERFACE_DIR) -lhttp_interface -lcjson -lm -lpthread -o $(TARGET)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	rm -f $(SEARCH_OBJ)
	rm -f $(TARGET)
