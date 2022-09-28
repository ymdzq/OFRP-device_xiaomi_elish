#
# Copyright (C) 2022 The Android Open Source Project
# Copyright (C) 2022 SebaUbuntu's TWRP device tree generator
#
# SPDX-License-Identifier: Apache-2.0
#

# Release name
PRODUCT_RELEASE_NAME := elish

# 引用默认的编译配置文件，core_64_bit必须先于其他引用，否则不生效
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_minimal.mk)

# Enable virtual A/B OTA
$(call inherit-product, $(SRC_TARGET_DIR)/product/virtual_ab_ota.mk)

# Installs gsi keys into ramdisk, to boot a developer GSI with verified boot.
$(call inherit-product, $(SRC_TARGET_DIR)/product/gsi_keys.mk)

# Inherit some common TWRP stuff.
$(call inherit-product, vendor/twrp/config/common.mk)

# Inherit from elish device
$(call inherit-product, device/xiaomi/elish/device.mk)

# Device identifier. This must come after all inclusions
PRODUCT_DEVICE := elish
PRODUCT_NAME := twrp_elish
PRODUCT_BRAND := Xiaomi
PRODUCT_MODEL := M2105K81AC
PRODUCT_MANUFACTURER := Xiaomi
#PRODUCT_RELEASE_NAME := Xiaomi Pad 5 Pro

PRODUCT_GMS_CLIENTID_BASE := android-xiaomi

PRODUCT_BUILD_PROP_OVERRIDES += \
    PRIVATE_BUILD_DESC="elish-user 12 RKQ1.211001.001 22.9.26 release-keys"

BUILD_FINGERPRINT := Xiaomi/elish/elish:12/RKQ1.211001.001/22.9.26:user/release-keys
