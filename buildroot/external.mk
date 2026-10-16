# Buildroot will look for this file, we need to point it to cat-gif.mk
include $(sort $(wildcard $(BR2_EXTERNAL_CATGIF_PATH)/package/*/*.mk))
