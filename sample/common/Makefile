PORTING_ROOT_DIR ?= $(shell cd $(CURDIR)/../.. && /bin/pwd)

include $(PORTING_ROOT_DIR)/porting_base.mk

LIB_NAME := libhi_sample_common

THIS_LIB_SHARE_INSTALL_DIR := $(PORTING_ROOT_DIR)/lib
THIS_LIB_STATIC_INSTALL_DIR := $(PORTING_ROOT_DIR)/lib

SDK_USR_CFLAGS += $(PORTING_CFLAGS)
SDK_USR_CFLAGS += -I$(PORTING_ROOT_DIR)/sample/audio/adp

SRCS := sample_comm_isp.c \
	sample_comm_sys.c \
	sample_comm_vi.c \
	sample_comm_vpss.c \
	sample_comm_2bpp_drawline.c \
	sample_comm_region.c \
	sample_comm_venc.c \
	sample_comm_vo.c \
	sample_comm_audio.c \
	loadbmp.c

include $(SDK_DIR)/build/sdk_lib_rules.mk
