include ../tool.mk

VEHICLE_DETECT:=$(PWD)
VEHICLE_DETECT_SUBDIRS := $(shell find $(VEHICLE_DETECT) -type d)
VEHICLE_DETECT_SRC := $(foreach dir,$(VEHICLE_DETECT_SUBDIRS),$(wildcard $(dir)/*.cpp))
VEHICLE_DETECT_OBJ := $(patsubst %.cpp,%.o, $(VEHICLE_DETECT_SRC))

NCNN_INC=../ncnn/include/ncnn/
NCNN_LIB=../ncnn/lib

CPPFLAGS = -I$(NCNN_INC)

TARGET=libvehicle_detect.a
all:$(TARGET)
$(TARGET): $(VEHICLE_DETECT_OBJ)
	$(AR) crv $(TARGET) $(VEHICLE_DETECT_OBJ)

%.o : %.cpp
	$(CPP) -c $(CPPFLAGS) $< -o $@
clean:
	rm -f $(VEHICLE_DETECT_OBJ)
	rm -f $(TARGET)
