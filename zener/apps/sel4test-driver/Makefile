#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

# Targets
TARGETS := $(notdir $(SOURCE_DIR)).bin

# Set custom entry point as we are a rootserver and will not be
# started in a standard way
ENTRY_POINT := _sel4_start

# Source files required to build the target
CFILES   := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.c))
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/plat/${PLAT}/*.c))
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch/${ARCH}/*.c))

# CPIO archive
OFILES := archive.o

# Add test_names.c manually because it is a generated file that will not be
# seen.
CFILES   += ${BUILD_DIR}/src/test_names.c

ASMFILES := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch/${ARCH}/*.S))
ASMFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/plat/${PLAT}/*.S))

# Libraries required to build the target
LIBS = c sel4 cpio elf sel4muslcsys sel4vka sel4allocman \
       platsupport sel4platsupport sel4test sel4vspace \
       sel4utils utils sel4simple
#ifdef CONFIG_KERNEL_STABLE
#LIBS += sel4simple-stable
#else
#LIBS += sel4simple-default
#endif

# extra cflag for sel4test
CFLAGS += -Werror -g
ifdef CONFIG_X86_64
CFLAGS += -mno-sse
endif

include $(SEL4_COMMON)/common.mk

# targets to generate CPIO archive of elf files
${COMPONENTS}:
	false

archive.o: ${COMPONENTS}
	$(Q)mkdir -p $(dir $@)
	${COMMON_PATH}/files_to_obj.sh $@ _cpio_archive $^

# generate test names file from tests app
${BUILD_DIR}/src/test_names.c: ${COMPONENTS}
	@echo "[GEN] test_names.c"
	${Q}mkdir -p $(dir $@)
	${SOURCE_DIR}/scripts/extract-test-names.sh $(TOOLPREFIX)objdump ${BUILD_DIR}/../sel4test-tests/sel4test-tests.bin > $@


