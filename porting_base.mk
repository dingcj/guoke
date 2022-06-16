SDK_DIR := $(shell cd $(PORTING_ROOT_DIR)/GKIPCLinuxV100R001C00SPC030 && /bin/pwd)

include $(SDK_DIR)/build/base.mk

PORTING_LIB_MODE := static
#PORTING_LIB_MODE := share

SENSOR0_TYPE := GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT

PORTING_CFLAGS += $(SDK_USR_CFLAGS)
PORTING_CFLAGS += -DUSER_BIT_32 -DKERNEL_BIT_32 -Wno-date-time -D_GNU_SOURCE
PORTING_CFLAGS += -DSENSOR0_TYPE=$(SENSOR0_TYPE) -DHI_ACODEC_TYPE_INNER -DHI_VQE_USE_STATIC_MODULE_REGISTER -DHI_AAC_USE_STATIC_MODULE_REGISTER -DHI_AAC_HAVE_SBR_LIB -DCHIPSET=$(CHIPSET)

PORTING_CFLAGS += -I$(GMP_DIR)/include \
            	  -I$(GMP_DIR)/modules/osal/include \
            	  -I$(PORTING_ROOT_DIR)/include \
                  -I$(PORTING_ROOT_DIR)/sample/common

SYS_LIBS := -lpthread -lm -lc -lrt -ldl -lstdc++

# gmp base
GMP_LIBS := -lsecurec -laac_comm -ldehaze -ldnvqe -ldrc -lir_auto -lldci -lupvqe -lvoice_engine
GMP_LIBS += -lsns_ar0237 -lsns_f37 -lsns_gc2053 -lsns_gc2053_forcar -lsns_imx290 -lsns_imx307_2l -lsns_imx307
GMP_LIBS += -lsns_imx327_2l -lsns_imx327 -lsns_imx335 -lsns_os05a -lsns_ov2718 -lsns_sc2231 -lsns_sc2235 -lsns_sc3235 -lsns_sc4236
GMP_LIBS += -lsns_sc3335 -lsns_sc500ai -lsns_gc4653_2l

# gmp api
GMP_LIBS += -laac_dec -laac_enc -laac_sbr_dec -laac_sbr_enc -lgk_ae -lgk_api -lgk_awb -lgk_bcd
GMP_LIBS += -lgk_cipher -lgk_isp -lgk_ive -lgk_ivp -lgk_md -lgk_qr -lgk_tde
GMP_LIBS += -lvqe_aec -lvqe_agc -lvqe_anr -lvqe_eq -lvqe_hpf -lvqe_record -lvqe_res -lvqe_talkv2 -lvqe_wnr

# porting library
HI_LIBS += -lhi_aacdec -lhi_aacsbrdec -lhi_ae -lhi_bcd -lhi_isp -lhi_ivp -lhi_mpi -lhi_tde -lhi_vqe_agc -lhi_vqe_eq  -lhi_vqe_record -lhi_vqe_talkv2
HI_LIBS += -lhi_aacenc -lhi_aacsbrenc -lhi_awb -lhi_cipher -lhi_ive -lhi_md -lhi_qr -lhi_vqe_aec -lhi_vqe_anr -lhi_vqe_hpf -lhi_vqe_res -lhi_vqe_wnr
HI_LIBS += -lhi_sample_common

LIBS_PATH := -L$(PORTING_ROOT_DIR)/lib -L$(PORTING_ROOT_DIR)/sample/common

ifeq ($(PORTING_LIB_MODE), static)
PORTING_LIBS += -L$(GK_STATIC_LIB_DIR) $(LIBS_PATH)
PORTING_LIBS += -Wl,-Bstatic,--start-group $(GMP_LIBS) $(HI_LIBS) -Wl,--end-group,-Bdynamic $(SYS_LIBS)
else ifeq ($(PORTING_LIB_MODE), share)
PORTING_LIBS += -L$(GK_SHARED_LIB_DIR) $(LIBS_PATH)
PORTING_LIBS += -shared -Wl,--start-group $(GMP_LIBS) $(HI_LIBS) $(SYS_LIBS) -Wl,--end-group
endif
