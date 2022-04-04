include ../tool.mk

PLATE_RECOG:=$(PWD)
PLATE_RECOG_SUBDIRS := $(shell find $(PLATE_RECOG) -type d)
PLATE_RECOG_SRC := $(foreach dir,$(PLATE_RECOG_SUBDIRS),$(wildcard $(dir)/*.c))
PLATE_RECOG_OBJ := $(patsubst %.c,%.o, $(PLATE_RECOG_SRC))

CFLAGS=-I$(PLATE_RECOG)

TARGET=libplate_recog.a
all:$(TARGET)
$(TARGET): $(PLATE_RECOG_OBJ)
	$(AR) crv $(TARGET) $(PLATE_RECOG_OBJ)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	rm -f $(PLATE_RECOG_OBJ)
	rm -f $(TARGET)
