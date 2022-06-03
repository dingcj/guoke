
objects := audio \
	   venc

objects_clean := $(addsuffix _clean,$(objects))

.PHONY: all clean $(objects) $(objects_clean) common common_clean

all: $(objects)

clean: $(objects_clean)

$(objects): common
	$(AT)make -C $@

$(objects_clean): common_clean
	$(AT)make -C $(patsubst %_clean,%,$@) clean

common:
	$(AT)make -C common

common_clean:
	$(AT)make -C common clean

eqm: audio venc_eqm

venc_eqm: common
	cd venc;$(AT)make -f Makefile_eqm;cd ..
