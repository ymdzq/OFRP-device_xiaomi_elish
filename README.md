# 橙 狐 (OFRP) for Xiaomi Pad 5 Pro (elish)  
使用小米平板 5 Pro，HyperOS1.0（安卓13）制作，适用于橙狐安卓12分支  

![OFRP](https://image.ibb.co/cTMWux/logo.jpg "OFRP")  
====================================================
# 目前进度
屏幕画面可以正常显示  
为了保证ui正常不错位，屏幕左右有留空  
recovery基本功能都可以正常使用  
刷入HyperOS（安卓15）之后，如果出现/data解密失败的问题，开机后设置一个锁屏密码，然后就好了  
# 如何使用
进入[Release](https://github.com/ymdzq/OFRP-device_xiaomi_elish/releases)中，点开Assets选项，点击7z压缩包文件名下载  
解压所有文件后，打开解压出的文件夹，运行recovery-twrp一键刷入工具.bat根据提示刷入，如果adb连接设备成功会自动重启进入recovery  
刷入工具脚本，感谢wzsx150大佬  

临时启动成功之后，可以通过“菜单”>“更多”>“安装当前OrangeFox”>滑动滑块确认，把橙狐固化进boot分区，替换掉官方recovery，  
也可以通过刷入zip格式的橙狐安装包来完成固化，  
固化会替换掉当前的ramdisk，所以magisk的root权限会掉，  
需要刷入magisk的zip重新root，  
或者备份当前boot之后用备份出来的boot文件重新修补root，即可实现橙狐、magisk共存。  
apatch不受固化影响，安装顺序不分先后。  

温馨提示：  
vab设备刷入rom之后会设置下次启动另一个槽位，需要重启生效，  
比如你当前系统在a槽，rom会刷入b槽，之后需要先重启手机，启动b槽  
如果未重启直接在rec里刷入面具，会直接刷进a槽，b槽开机后仍无root权限  

偷懒可以试试用搞机助手电脑版、FastbootEnhance这种软件配合刷机  
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
# 云编译
利用Github Action在线编译橙狐  
例如你的 Github 用户名是 "JohnSmith"  
1. 打开[橙狐Action编译器](https://github.com/ymdzq/OrangeFox-Action-Builder)仓库，然后在新页面点击右上角的`Fork`按钮  
![image](https://user-images.githubusercontent.com/37921907/177914706-c92476c5-7e14-4fb3-be94-0c8a11dae874.png)
2. 等待网页自动重定向后，你将会看到你的用户名下的新仓库  
![image](https://user-images.githubusercontent.com/37921907/177915106-5bde6fc9-303c-479e-b290-22b48efd1e4e.png)
3. 网页上方进入 `Actions` 页面 > `All workflows` > `OrangeFox - Build` > `Run workflow`  
![image](https://user-images.githubusercontent.com/37921907/177915304-8731ed80-1d49-48c9-9848-70d0ac8f2720.png)
4. 按照以下内容填写参数  
OrangeFox Branch  
`12.1`  
Custom Recovery Tree  
`https://github.com/ymdzq/OFRP-device_xiaomi_elish`  
Custom Recovery Tree Branch  
`fox_12.1-a16`  
Specify your device path.  
`device/xiaomi/elish`  
Specify your Device Codename.  
`elish`  
Specify your Build Target  
`boot`  
![image](https://user-images.githubusercontent.com/37921907/177915346-71c29149-78fb-4a00-996f-5d84ffc9eb8c.png)
5. 填写完毕后, 点击 "Run workflow" 开始运行
6. 编译结果可以在你Fork后的新仓库的Release页面下载
