# 橙 狐 (OFRP) for Xiaomi Pad 5 Pro (elish)
使用小米平板 5 Pro，MIUI 14（安卓13）制作，提取官方安卓13内核，适用于橙狐安卓12分支

![OFRP](https://image.ibb.co/cTMWux/logo.jpg "OFRP")

====================================================

# 目前进度

屏幕大部分无画面显示，只有状态栏一条线亮

由于平板无振动功能，屏幕又不显示画面，无法确定触屏是否有效

recovery基本功能都可以正常使用，你是要让我用adb shell twrp命令行刷机？

使用小米平板 5 Pro制作，理论上支持小米平板 5 Pro 5G，等一个勇士来测试

由于屏幕问题无解，目前只能到此为止
已知非内核问题，使用IMMENSITY内核启动系统可以正常运行，
平板5p比较k40除了多了杜比视界和一堆audio驱动，少了震动和nfc相关文件外，没有明显的软件不同
所以感觉要从魔改twrp的界面显示功能上去着手处理，等一个大佬

# 如何使用

进入[Release](https://github.com/ymdzq/OFRP-device_xiaomi_elish/releases)中，根据介绍使用。

# 如何构建
下载OFRP源代码，克隆这个仓库放到相应的位置

例如OFRP源代码根目录为~/fox_12.1，则保存为~/fox_12.1/device/xiaomi/elish/:

```bash
cd ~/fox_12.1
mkdir -p device/xiaomi
cd device/xiaomi
git clone https://github.com/ymdzq/OFRP-device_xiaomi_elish.git elish
```

打开源代码根目录运行:

```bash
. build/envsetup.sh && lunch twrp_elish-eng && mka bootimage
```
