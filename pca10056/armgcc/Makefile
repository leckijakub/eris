PROJECT_NAME     := espar_custom_pca10056
TARGETS          := nrf52840_xxaa
OUTPUT_DIRECTORY := _build

SDK_ROOT :=../../../..
PROJ_DIR := ../..

$(OUTPUT_DIRECTORY)/nrf52840_xxaa.out: \
  LINKER_SCRIPT  := armgcc_10056_nrf52840_xxaa.ld

CFLAGS += -DBOARD_CUSTOM -DBOARD_DD
# COMMON --- 
include $(PROJ_DIR)/config/common.mk
