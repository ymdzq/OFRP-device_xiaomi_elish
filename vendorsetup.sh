#
# Copyright (C) 2023 The Android Open Source Project
# Copyright (C) 2023 SebaUbuntu's TWRP device tree generator
#
# SPDX-License-Identifier: Apache-2.0
#
# 关于橙狐变量详见 OrangeFox_10/fox_10.0/vendor/recovery/orangefox_build_vars.txt

	export LC_ALL="C"
	export ALLOW_MISSING_DEPENDENCIES=true

	echo -e "\x1b[96melish: 开始添加OrangeFox Vars...\x1b[m"
	## 构建信息
	# 设置显示在关于页面里的维护人员名称
	export OF_MAINTAINER=ymdzq
	# 设置版本号为日期
	export FOX_VERSION=$(date +%y.%m.%d)

	## 添加功能
	# 使用完整版ps命令
	export FOX_REPLACE_BUSYBOX_PS=1
	# 使用完整版getprop命令
	export FOX_REPLACE_TOOLBOX_GETPROP=1
	# 支持tar命令
	export FOX_USE_TAR_BINARY=1
	# 支持sed命令
	export FOX_USE_SED_BINARY=1
	# 使用bash代替sh和ash
	export FOX_USE_BASH_SHELL=1
	export FOX_ASH_IS_BASH=1
	# 使用完整版grep命令
	export FOX_USE_GREP_BINARY=1
	# 支持lzma, xz命令
	export FOX_USE_XZ_UTILS=1
	# 使用橙狐编译的NANO编辑器
	export FOX_USE_NANO_EDITOR=1
	# 添加phhusson's lptools工具
	export OF_ENABLE_LPTOOLS=1

	## 添加橙狐特殊处理
	# 设定recovery、system、vendor、boot分区路径
	# export FOX_RECOVERY_INSTALL_PARTITION="/dev/block/bootdevice/by-name/recovery"
	export FOX_RECOVERY_SYSTEM_PARTITION="/dev/block/mapper/system"
	export FOX_RECOVERY_VENDOR_PARTITION="/dev/block/mapper/vendor"
	# export FOX_RECOVERY_BOOT_PARTITION="/dev/block/bootdevice/by-name/boot"

	# 当安装MIUI或者在MIUI上安装橙狐zip，使用magisk处理所有boot和recovery镜像，防止橙狐被MIUI官方recovery替换
	export OF_FORCE_MAGISKBOOT_BOOT_PATCH_MIUI=1
	# 避免在已加密设备上应用强制加密补丁
	export OF_DONT_PATCH_ENCRYPTED_DEVICE=1
	# 跳过FBE解密流程（防止卡在橙狐LOGO或Redmi/Mi LOGO界面）
	# export OF_SKIP_FBE_DECRYPTION=1
	# 当ROM大于等于指定安卓SDK等级时，跳过FBE解密流程
	# export OF_SKIP_FBE_DECRYPTION_SDKVERSION=31
	# 在MIUI OTA还原期间尝试解密内部存储（而不是错误退出）
	export OF_OTA_RES_DECRYPT=1
	# 在准备MIUI OTA增量更新时不备份vendor_image
	export OF_NO_MIUI_OTA_VENDOR_BACKUP=1
	# 防止橙狐在解密后重新运行自启动进程
	export OF_NO_RELOAD_AFTER_DECRYPTION=1
	# 使用CAF内核
	export USE_CAF_KERNEL=1
	## 硬件功能设定
	# 没有绿色led
	export OF_USE_GREEN_LED=0
	# 关闭闪光灯功能
	# export OF_FLASHLIGHT_ENABLE=0
	# 自定义闪光灯路径，修复闪光灯
	# export OF_FL_PATH1=/sys/class/leds/led:torch_0
	# export OF_FL_PATH2=/sys/class/leds/led:torch_1

	## 界面显示设定
	# 设置屏幕高度，状态栏高度，状态栏左右边距
	# 由于橙狐默认屏幕比例是16：9,所以应设置屏幕高度为屏幕比例换算成n：9之后，n*120
	export OF_SCREEN_H=1728
	# export OF_STATUS_H=60
	# export OF_STATUS_INDENT_LEFT=48
	# export OF_STATUS_INDENT_RIGHT=48
	# 添加黑色状态栏（隐藏刘海）选项
	# export OF_HIDE_NOTCH=1
	# 由于有刘海遮挡，设置时钟位置为只能显示在左侧或右侧
	# export OF_CLOCK_POS=1
	# 禁止禁用导航栏
	export OF_ALLOW_DISABLE_NAVBAR=0

	## 使刷机包兼容小米平板5 Pro和小米平板5 Pro 5G
	# 设置一个很老的build时间，用于解决某些ROM例如MIUI刷机脚本里的防回滚保护检测
	export FOX_BUGGED_AOSP_ARB_WORKAROUND="1546300800"; # Tuesday, January 1, 2019 12:00:00 AM GMT+00:00
	# 使小米平板5 Pro和小米平板5 Pro 5G都能刷入橙狐zip卡刷包
	export TARGET_DEVICE_ALT="elish, enuma"
	# 使橙狐可以刷入具有机型检测限制为小米平板5 Pro或者小米平板5 Pro 5G的zip卡刷包，与TARGET_OTA_ASSERT_DEVICE冲突
	# export OF_TARGET_DEVICES="elish,enuma"

	# 尝试处理AVB2.0，防止橙狐被官方recovery替换
	# export OF_PATCH_AVB20=1
	# 使用指定的magisk
	# export FOX_USE_SPECIFIC_MAGISK_ZIP="$HOME/Magisk.zip"
	# 使用指定的magisk版本号，由于magisk 23+使用了新的包装形式，文件路径改变了，橙狐无法获取正确的版本
	# export MAGISK_VER=25.2
	# 当修补recovery/boot镜像时，始终指示magiskboot v24+修补vbmeta标头（自动禁用avb验证？）
	export OF_PATCH_VBMETA_FLAG=1
	# 标记该设备肯定是原生Android 11+虚拟A/B（“VAB”）设备
	export OF_VIRTUAL_AB_DEVICE=1
	# 指定super分区的完整大小，修复部分设备刷入miui rom失败，VAB设备专用变量
	export OF_DYNAMIC_FULL_SIZE=9126805504
	# ensure that /sdcard is bind-unmounted before f2fs data repair or format
	export OF_UNBIND_SDCARD_F2FS=1

	echo -e "\x1b[96melish: 当你看到这个消息的时候，所有的OrangeFox Var已经添加完毕！\x1b[m"
