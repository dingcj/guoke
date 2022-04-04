include ../tool.mk

CTRLCGI:=$(PWD)
CTRLCGI_SUBDIRS := $(shell find $(CTRLCGI) -type d)
CTRLCGI_SRC := $(foreach dir,$(CTRLCGI_SUBDIRS),$(wildcard $(dir)/*.c))
CTRLCGI_OBJ := $(patsubst %.c,%.o, $(CTRLCGI_SRC))

CJSON=../cjson
HTTP_INTERFACE_DIR=../HttpInterface
CFLAGS=-I$(CJSON) -I$(HTTP_INTERFACE_DIR)

TARGET=ctrcgi
all:$(TARGET)
$(TARGET): $(CTRLCGI_OBJ)
	@cd $(CJSON);make
	@$(CC) $(CTRLCGI_OBJ) -L$(CJSON) -L$(HTTP_INTERFACE_DIR) -lhttp_interface -lcjson -lm -lpthread -o $(TARGET)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	rm -f $(CTRLCGI_OBJ)
	rm -f $(TARGET)
