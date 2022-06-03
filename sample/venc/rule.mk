SRCS := $(wildcard ./*.c)

OBJS := $(patsubst %.c, %.o, $(SRCS))

.PHONY: all clean

all: $(OBJS)
	@cd $(PARKING_GUIDANCE_LIB_DIR);make
	@cd $(PLATE_RECOG);make
	@cd $(VEHICLE_DETECT_DIR);make
	@cd $(HTTP_INTERFACE_DIR);make
	@cd $(RTSP_DIR);make
	@cd $(CJSON_DIR);make
	@cd $(CTRLCGI_DIR);make
	@cd $(SEARCH_DIR);make
	@cd $(BOA_DIR);make

	$(AT)$(CC) -o $(TARGET) $^ $(CFLAGS)

%.o : %.c
	$(AT)$(CC) -c -o $@ $< $(CFLAGS)

clean:
	@cd $(PARKING_GUIDANCE_LIB_DIR);make clean
	@cd $(PLATE_RECOG);make clean
	@cd $(VEHICLE_DETECT_DIR);make clean
	@cd $(HTTP_INTERFACE_DIR);make clean
	@cd $(RTSP_DIR);make clean
	@cd $(CJSON_DIR);make clean
	@cd $(CTRLCGI_DIR);make clean
	@cd $(SEARCH_DIR);make clean
	@cd $(BOA_DIR);make clean
	$(AT)rm -rf $(OBJS) $(TARGET)

