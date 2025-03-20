# WARNING:仅基于逐飞方案，使用逐飞内核，使用逐飞扩展板或屏幕引脚与逐飞扩展板相同可用该工具
# wifi_setting_tool
可以通过读取wpa_supplicant.conf控制多个wifi的连接，连接完后第一次按p13还会进入程序附带的ip_show_tool IP显示工具，方便查看IP（详见我的ip_show_tool仓库）
# 项目说明：
- 基于逐飞开源库开发的wifi选择与连接工具，可以往板卡上配置多个wifi信息并在开机时选择wifi进行连接，使用逐飞屏幕库进行gui设计，可视化强，便于配置wifi
- 附：什么叫做我开发的，分明是deepseek开发我迁移的（ds负责了读取wpa_supplicant.conf，列出wifi，连接wifi，我写了选择wifi与gui化）
- 感谢deepseek开源.jpg
- 这个群就我没本领.gif
- 鸣谢：某几个ACM仙人给我补的c与c++知识
- 我是菜逼.gif
# 使用方法
- 1.从release下载wifi_setting_tool
- 2.wifi/网线ssh连接板子
- 3.将wifi_setting_tool拷进久久派中的/home/root
- 4.cd /etc/
- 5.vi wpa_supplicant.conf
- 此时你的界面大概会显示这堆东西
```
ctrl_interface=/var/run/wpa_supplicant
ctrl_interface_group=0
update_config=1

network={
        一个wifi的账密以及其他信息，忘了
}

~
~
~
~
~
~
~
~
- wpa_supplicant.conf 1/14 7%
```
- 接下来教大家如何配置这堆network块
- 每当你要添加一个wifi的信息，你就要写一个network块进这个wpa_supplicant.conf
- 照着如下写：
```
network={
      ssid="这是你要连的wifi的名字，别**抄这堆汉字上去，用你那wifi名字替换这句话"
      psk="这是你的密码"
      key_mgnt=这是你的wifi加密方式，别**加引号
      以及其他乱七八糟的配置
}
```
- 关于这个加密方式，以下贴一篇文章:[Linux 命令行调试WiFi](https://blog.csdn.net/ylxwk/article/details/135116188)
- 下面贴示例
- 以下是一个公共wifi的配置
```
network={
        ssid="那个公共wifi的名字"
        key_mgmt=NONE
}
```
- 以下是一个使用wpa2加密的wifi配置（例如手机热点）
```
network={
        ssid="wifi名。"
        psk="密码。"
        （下面就没东西了，因为wpa2根本不用写key_mgnt）（还是那句话，别把这堆玩意抄上去了）
}
```
- 最终我的wpa_supplicant.conf长这样
```
ctrl_interface=/var/run/wpa_supplicant
ctrl_interface_group=0
update_config=1

network={
        ssid="我那个热点的名字"
        psk="密码"
}

network={
        ssid="一个公共wifi的名字"
        key_mgmt=NONE
}
```
- 至少对于你第一次配置是这样
- 在我这个wifi_setting_tool运行过后，你没连的wifi会被标记disable
- 所以其实我现在的wpa_supplicant.conf其实长这样
```
ctrl_interface=/var/run/wpa_supplicant
ctrl_interface_group=0
update_config=1

network={
        ssid="我那个热点的名字"
        psk="密码"
}

network={
        ssid="一个公共wifi的名字"
        key_mgmt=NONE
        disabled=1
}
```
- 这是正常现象
- 在配完你的wpa_supplicant.conf之后，按esc，然后:wq保存退出，进入第六步
- 6.vi rc.local（你现在还在/etc/，不是吗）
- 直接抄，复制粘贴
```
#! /bin/bash -e

do_start()
{
#systemctl disable dhcpcd
#systemctl stop dhcpcd
insmod /usr/lib/modules/4.19.190+/aic8800_bsp.ko
insmod /usr/lib/modules/4.19.190+/aic8800_fdrv.ko

sleep 3

#hostapd -B /etc/hostapd.conf -f /var/log/hstap.log &
#udhcpd -f /etc/udhcpd.conf &
home/root/wifi_setting_tool &
}
do_stop()
{
rmmod aic8800_fdrv.ko
rmmod aic8800_bsp.ko
}
case "$1" in
   start)
     do_start
   ;;
   stop)
     do_stop
   ;;
 esac

exit 0
```
- 放心，连udhcpc -i wlan0 &都不用加，这个程序已经帮你搞定了联网所需的一切
- 确认一下你/home/root里头有那个wifi_setting_tool后，你就可以
- sync
- reboot
- 了
- 重启后，该工具自动运行，观察逐飞扩展板，会发现最左侧（靠近电源开关）有四个按钮
- 从上到下分别为
- 丝印p16 向上选择
- 丝印p15 向下选择
- 丝印p14 确认并连接wifi（由于某些幺蛾子，你开机时必须按一次这个按键才会联网）
- 丝印p13 退出（长按0.2s）
- 开机后，通过p16与p15选择你想连接的wifi（就是你刚刚修改的wpa_supplicant.conf里面的那些配置）
- 然后按p14连接
- 连接完后按p13 0.2s以上，退出wifi连接功能
- 松手进入ip查询与显示功能，此时会查询板卡的eth0与wlan0的信息并显示到屏幕上，就是之前我那个ip_show_tool仓库程序的功能
- 再度长按p13 0.2s，屏幕变白后即代表程序已退出，可以运行你的循迹代码了
