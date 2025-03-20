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
}
关于这个加密方式，以下贴一篇文章:(https://blog.csdn.net/ylxwk/article/details/135116188)
