PARKING_GUIDANCE_LIB_DIR ?= $(shell cd ../ParkingGuidanceLib && /bin/pwd)
PLATE_RECOG ?= $(shell cd ../PlateRecog && /bin/pwd)
VEHICLE_DETECT_DIR ?= $(shell cd ../VehicleDetect && /bin/pwd)
NCNN_DIR ?= $(shell cd ../ncnn && /bin/pwd)
HTTP_INTERFACE_DIR ?= $(shell cd ../HttpInterface && /bin/pwd)
LIB_CURL_DIR ?= $(shell cd ../libcurl && /bin/pwd)
RTSP_DIR ?= $(shell cd ../rtsp && /bin/pwd)
CJSON_DIR ?= $(shell cd ../cjson && /bin/pwd)
CTRLCGI_DIR ?= $(shell cd ../ctrlCgi && /bin/pwd)
SEARCH_DIR ?= $(shell cd ../search && /bin/pwd)
BOA_DIR ?= $(shell cd ../boa-0.94.13/src && /bin/pwd)

PORTING_CFLAGS += -I$(REL_INC) \
		  -I$(PARKING_GUIDANCE_LIB_DIR) \
		  -I$(HTTP_INTERFACE_DIR) \
		  -I$(RTSP_DIR) \
		  -I$(CJSON_DIR) \
		  -I$(LIB_CURL_DIR)/include

LIBS_PATH += -L$(PARKING_GUIDANCE_LIB_DIR) -lparking_guidance \
	     -L$(PLATE_RECOG) -lplate_recog \
	     -L$(VEHICLE_DETECT_DIR) -lvehicle_detect \
	     -L$(NCNN_DIR)/lib -lncnn \
	     -L$(HTTP_INTERFACE_DIR) -lhttp_interface \
	     -L$(LIB_CURL_DIR)/lib -lcurl \
	     -L$(RTSP_DIR) -lrtsp \
	     -L$(CJSON_DIR) -lcjson

objects := audio \
	   venc

objects_clean := $(addsuffix _clean,$(objects))

.PHONY: all clean $(objects) $(objects_clean) common common_clean

all: $(objects)

clean: $(objects_clean)

$(objects): common
	@cd $(PARKING_GUIDANCE_LIB_DIR);make
	@cd $(PLATE_RECOG);make
	@cd $(VEHICLE_DETECT_DIR);make
	@cd $(HTTP_INTERFACE_DIR);make
	@cd $(RTSP_DIR);make
	@cd $(CJSON_DIR);make
	@cd $(CTRLCGI_DIR);make
	@cd $(SEARCH_DIR);make
	@cd $(BOA_DIR);make
	$(AT)make -C $@

$(objects_clean): common_clean
	@cd $(PARKING_GUIDANCE_LIB_DIR);make clean
	@cd $(PLATE_RECOG);make clean
	@cd $(VEHICLE_DETECT_DIR);make clean
	@cd $(HTTP_INTERFACE_DIR);make clean
	@cd $(RTSP_DIR);make clean
	@cd $(CJSON_DIR);make clean
	@cd $(CTRLCGI_DIR);make clean
	@cd $(SEARCH_DIR);make clean
	@cd $(BOA_DIR);make clean
	$(AT)make -C $(patsubst %_clean,%,$@) clean

common:
	$(AT)make -C common

common_clean:
	$(AT)make -C common clean
