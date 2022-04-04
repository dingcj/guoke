include ../tool.mk

PARKING_GUIDANCE:=$(PWD)
PARKING_GUIDANCE_SUBDIRS := $(shell find $(PARKING_GUIDANCE) -type d)
PARKING_GUIDANCE_SRC := $(foreach dir,$(PARKING_GUIDANCE_SUBDIRS),$(wildcard $(dir)/*.cpp))
PARKING_GUIDANCE_OBJ := $(patsubst %.cpp,%.o, $(PARKING_GUIDANCE_SRC))

VEHICLE_DETECT_INC=../VehicleDetect
PLATE_INC=../PlateRecog
CPPFLAGS=-I$(VEHICLE_DETECT_INC) -I$(PLATE_INC)

TARGET=libparking_guidance.a
all:$(TARGET)
$(TARGET): $(PARKING_GUIDANCE_OBJ)
	$(AR) crv $(TARGET) $(PARKING_GUIDANCE_OBJ)

%.o : %.cpp
	$(CPP) -c $(CPPFLAGS) $< -o $@
clean:
	rm -f $(PARKING_GUIDANCE_OBJ)
	rm -f $(TARGET)
