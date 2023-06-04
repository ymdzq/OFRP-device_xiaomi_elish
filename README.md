# 橙 狐 (OFRP) for Xiaomi Pad 5 Pro (elish)  
使用小米平板 5 Pro，MIUI 14（安卓13）制作，适用于橙狐安卓12分支  

![OFRP](https://image.ibb.co/cTMWux/logo.jpg "OFRP")  
====================================================
# 目前进度
屏幕画面可以显示，但是有条纹闪烁  
锁屏后触屏失效，直到重启  
recovery基本功能都可以正常使用  
目前看来是由于twrp不支持屏幕dfps功能的问题，使用原装内核无法点亮屏幕，  
修改内核才能点亮屏幕，但由于内核变了，无法永久刷入机器，只支持临时启动  
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
