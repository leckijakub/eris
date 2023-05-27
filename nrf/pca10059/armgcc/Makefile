PROJECT_NAME     := espar_dongle_pca10059
TARGETS          := nrf52840_xxaa
OUTPUT_DIRECTORY := _build

SDK_ROOT :=../../../..
PROJ_DIR := ../..

$(OUTPUT_DIRECTORY)/nrf52840_xxaa.out: \
  LINKER_SCRIPT  := armgcc_10059_nrf52840_xxaa.ld

CFLAGS += -DBOARD_PCA10059
# COMMON --- 
include $(PROJ_DIR)/config/common.mk

