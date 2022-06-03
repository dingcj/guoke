PORTING_ROOT_DIR ?= $(shell cd $(CURDIR)/../.. && /bin/pwd)

PARKING_GUIDANCE_LIB_DIR ?= $(shell cd ../../ParkingGuidanceLib && /bin/pwd)
PLATE_RECOG ?= $(shell cd ../../PlateRecog && /bin/pwd)
VEHICLE_DETECT_DIR ?= $(shell cd ../../VehicleDetect && /bin/pwd)
NCNN_DIR ?= $(shell cd ../../ncnn && /bin/pwd)
HTTP_INTERFACE_DIR ?= $(shell cd ../../HttpInterface && /bin/pwd)
LIB_CURL_DIR ?= $(shell cd ../../libcurl && /bin/pwd)
RTSP_DIR ?= $(shell cd ../../rtsp && /bin/pwd)
CJSON_DIR ?= $(shell cd ../../cjson && /bin/pwd)
CTRLCGI_DIR ?= $(shell cd ../../ctrlCgi && /bin/pwd)
SEARCH_DIR ?= $(shell cd ../../search && /bin/pwd)
BOA_DIR ?= $(shell cd ../../boa-0.94.13/src && /bin/pwd)

PORTING_CFLAGS += -I$(PARKING_GUIDANCE_LIB_DIR)
PORTING_CFLAGS += -I$(HTTP_INTERFACE_DIR)
PORTING_CFLAGS += -I$(CJSON_DIR)
PORTING_CFLAGS += -I$(RTSP_DIR)
PORTING_CFLAGS += -I$(LIB_CURL_DIR)/include

PORTING_LIBS += -L$(PARKING_GUIDANCE_LIB_DIR) -lparking_guidance \
	     -L$(PLATE_RECOG) -lplate_recog \
	     -L$(VEHICLE_DETECT_DIR) -lvehicle_detect \
	     -L$(NCNN_DIR)/lib -lncnn \
	     -L$(HTTP_INTERFACE_DIR) -lhttp_interface \
	     -L$(LIB_CURL_DIR)/lib -lcurl \
	     -L$(RTSP_DIR) -lrtsp \
	     -L$(CJSON_DIR) -lcjson

