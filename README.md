# 橙 狐 (OFRP) for Xiaomi Pad 5 Pro (elish)  
使用小米平板 5 Pro，MIUI 14（安卓13）制作，适用于橙狐安卓12分支  

![OFRP](https://image.ibb.co/cTMWux/logo.jpg "OFRP")  
====================================================
# 目前进度
屏幕画面可以显示，但是有条纹闪烁  
锁屏后再点亮触屏生效，条纹闪烁消失  
为了保证ui正常不错位，屏幕右半边留空  
recovery基本功能都可以正常使用  
目前看来是由于twrp不支持屏幕dfps功能的问题，使用原装内核无法点亮屏幕，  
修改内核才能点亮屏幕，但由于内核变了，无法永久刷入机器，只支持临时启动  
# 如何使用
进入[Release](https://github.com/ymdzq/OFRP-device_xiaomi_elish/releases)中，点开Assets选项，点击7z压缩包文件名下载  
解压所有文件后，打开解压出的文件夹，运行recovery-twrp一键刷入工具.bat根据提示刷入，如果adb连接设备成功会自动重启进入recovery  
刷入工具脚本，感谢wzsx150大佬  

因为内核是魔改过的版本，临时启动用的是改好的内核，固化会丢掉临时内核，就又会用回miui官方内核然后就点不亮  
第三方内核需要刷入配套的vendor_boot.img镜像至vendor_boot分区，否则可能假砖无法开机，只能长按10秒电源键重启  
而刷入橙狐后如果后续没有刷rom替换掉vendor_boot就重启进入系统，可能会影响当前系统稳定性  
小米平板5Pro目前没有MIUI 14可以正常使用的开源内核，所以做不到两全其美  
（小米能不能放出一个靠谱的最新系统的内核源码？开源的还是安卓11）  
建议在刷入之前最好利用root权限想办法在系统中备份vendor_boot分区，或者提取你原来的rom包里的vendor_boot镜像，以便在需要的时侯还原  

提供一个自用MIUI 14.0.4.0内核备份，包含14.0.4.0原版boot、dtbo、vendor_boot以及修补过的面具boot文件，方便恢复  
解压得到14.0.4.0、14.0.4.0_magisk两个文件夹，放入小米平板5Pro“内置存储/Fox/BACKUPS/数字字母组成的机器识别代码”的文件夹里（没有这个文件夹你可以先随便备份一次就有了），即可在recovery中识别备份，可勾选恢复相应分区  
https://www.123pan.com/s/fRptVv-i8K4.html 提取码:BP89  

温馨提示：  
vab设备刷入rom之后会设置下次启动另一个槽位，需要重启生效，  
比如你当前系统在a槽，rom会刷入b槽，之后需要先重启手机，启动b槽  
如果未重启直接在rec里刷入面具，会直接刷进a槽，b槽开机后仍无root权限  

偷懒可以试试用搞基助手电脑版、FastbootEnhance这种软件配合刷机  
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
# 关于内核
由于目前小米平板5Pro没有其他可用的开源内核，我使用的是[eva内核](https://github.com/mvaisakh/alioth)修改编译  
以下内容仅供学习交流，构建橙狐使用预先编译好的内核，所以无需重复编译内核  
修改内容来自这个commit  
原理：[禁用动态fps和设置刷新率为104，修复drm渲染问题](https://github.com/map220v/android_kernel_xiaomi_nabu/commit/90b916915508d3f2b5fe371f0ae29cc6080faf98)  
### 具体内核编译过程如下：  
新建eva-kernel文件夹用于下载eva内核源代码，  
例如在用户目录新建一个eva-kernel文件夹，初始化仓库  
```bash
mkdir -p ~/eva-kernel
cd ~/eva-kernel
repo init -u https://github.com/mvaisakh/android_kernel_manifest.git -b eva-xiaomi-4.19
```
同步源码  
```bash
repo sync --force-sync --no-clone-bundle --current-branch --no-tags -j$(nproc --all)
```
手动下载[编译脚本](https://github.com/ymdzq/scripts/blob/main/elish.sh)放进eva-kernel文件夹  
手动下载[修改补丁](https://github.com/ymdzq/scripts/blob/main/0001-HACK-Disable-dynamic-fps-and-set-refresh-rate-104.patch)放进eva-kernel/kernel/msm-4.19文件文件夹  
用`git apply`命令打补丁
```bash
cd ~/eva-kernel/kernel/msm-4.19/
git apply 0001-HACK-Disable-dynamic-fps-and-set-refresh-rate-104.patch
```
开始编译
```bash
cd ~/eva-kernel
./elish.sh
```
编译完成后内核文件在~/eva-kernel/ak3文件夹中  

最后提一句，理论上有更好的修改内核的方法：[检测到进入recovery环境则修复画面显示](https://github.com/Rohail33/Realking_kernel_nabu/commit/067af9d07203b4c979ebbc0c0f0339d242a11d38)  
这样用同一个内核就可以既正常启动系统又可以启动recovery，  
可惜代码不完善，实测不生效，进入recovery仍然是黑屏，  
所以如果有其他能制作平板内核的大佬建议可以改进一下这个方法
