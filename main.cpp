
/*********************************************************************************************************************
* LS2K0300 Opensourec Library 即（LS2K0300 开源库）是一个基于官方 SDK 接口的第三方开源库
* Copyright (c) 2022 SEEKFREE 逐飞科技
*
* 本文件是LS2K0300 开源库的一部分
*
* LS2K0300 开源库 是免费软件
* 您可以根据自由软件基金会发布的 GPL（GNU General Public License，即 GNU通用公共许可证）的条款
* 即 GPL 的第3版（即 GPL3.0）或（您选择的）任何后来的版本，重新发布和/或修改它
*
* 本开源库的发布是希望它能发挥作用，但并未对其作任何的保证
* 甚至没有隐含的适销性或适合特定用途的保证
* 更多细节请参见 GPL
*
* 您应该在收到本开源库的同时收到一份 GPL 的副本
* 如果没有，请参阅<https://www.gnu.org/licenses/>
*
* 额外注明：
* 本开源库使用 GPL3.0 开源许可证协议 以上许可申明为译文版本
* 许可申明英文版在 libraries/doc 文件夹下的 GPL3_permission_statement.txt 文件中
* 许可证副本在 libraries 文件夹下 即该文件夹下的 LICENSE 文件
* 欢迎各位使用并传播本程序 但修改内容时必须保留逐飞科技的版权声明（即本声明）
*
* 文件名称          main
* 公司名称          成都逐飞科技有限公司
* 适用平台          LS2K0300
* 店铺链接          https://seekfree.taobao.com/
*
* 修改记录
* 日期              作者           备注
* 2025-12-27        大W            first version
********************************************************************************************************************/
#include <iostream>
#include <cstring>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <regex>
#include "zf_common_headfile.hpp"

#define KEY_1_PATH        ZF_GPIO_KEY_1
#define KEY_2_PATH        ZF_GPIO_KEY_2
#define KEY_3_PATH        ZF_GPIO_KEY_3
#define KEY_4_PATH        ZF_GPIO_KEY_4

zf_driver_gpio  key_0(KEY_1_PATH, O_RDWR);
zf_driver_gpio  key_1(KEY_2_PATH, O_RDWR);
zf_driver_gpio  key_2(KEY_3_PATH, O_RDWR);
zf_driver_gpio  key_3(KEY_4_PATH, O_RDWR);

zf_device_ips200 ips200;

void sigint_handler(int signum) 
{
    printf("收到Ctrl+C，程序即将退出\n");
    exit(0);
}

void cleanup()
{
    // 需要先停止定时器线程，后面才能稳定关闭电机，电调，舵机等
    printf("程序异常退出，执行清理操作\n");
    // 关闭电机  
}
std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}
std::string get_wireless_interface() {
    // try {
    //     std::string output = exec("iwconfig 2>&1");
    //     std::istringstream iss(output);
    //     std::string line;
    //     while (getline(iss, line)) {
    //         if (line.find("IEEE 802.11") != std::string::npos) {
    //             return line.substr(0, line.find(" "));
    //         }
    //     }
    // } catch (...) {}
    return "wlan0"; // 默认值
}
struct NetworkInfo {
    std::string id;
    std::string ssid;
    std::string flags;
};
std::vector<NetworkInfo> get_networks(const std::string& interface) {
    std::vector<NetworkInfo> networks;
    std::string cmd = "wpa_cli -i " + interface + " list_networks";
    std::string output = exec(cmd.c_str());

    std::istringstream iss(output);
    std::string line;
    bool header_found = false;

    while (getline(iss, line)) {
        if (!header_found) {
            if (line.find("network id") != std::string::npos) {
                header_found = true;
            }
            continue;
        }
        std::regex re("\\t+");
        std::sregex_token_iterator it(line.begin(), line.end(), re, -1);
        std::vector<std::string> parts(it, {});

        if (parts.size() >= 3) {
            NetworkInfo info;
            info.id = parts[0];
            info.ssid = parts[1];
            info.flags = parts.size() >=4 ? parts[3] : "";
            networks.push_back(info);
        }
    }
    return networks;
}
bool select_network(const std::string& interface, const std::string& net_id) {
    std::string cmd = "wpa_cli -i " + interface + " select_network " + net_id;
    std::string result = exec(cmd.c_str());

    if (result.find("OK") != std::string::npos) {
        exec(("wpa_cli -i " + interface + " save_config").c_str());
        //cout << "已成功选择网络，正在连接..." << endl;
		exec("udhcpc -i wlan0");
        // 释放和获取IP地址
        system(("dhclient -r " + interface + " >/dev/null 2>&1").c_str());
        system(("dhclient " + interface + " >/dev/null 2>&1").c_str());
        return true;
    }
    return false;
}
std::vector<std::string> ips_wlan_list;
std::string interface = get_wireless_interface();
auto find_wifi(){
	ips_wlan_list.clear();
	auto networks = get_networks(interface);
	for (size_t i = 0; i < networks.size(); ++i) {
		std::string status = (networks[i].flags.find("CURRENT") != std::string::npos) ?
					   "当前连接" : "已保存";
		if(status == "当前连接")ips_wlan_list.push_back(std::to_string(i+1)+"."+networks[i].ssid+"(connect)");
		else ips_wlan_list.push_back(std::to_string(i+1)+"."+networks[i].ssid);
	}
	return networks;
}
void connect_wifi(std::vector<NetworkInfo> networks,int choice){
	if (select_network(interface, networks[choice-1].id)) {
		std::string status = exec(("wpa_cli -i " + interface + " status").c_str());
	}
}
char eth0[INET_ADDRSTRLEN];
int show_eth0_on_screen(){
		int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		struct ifreq ifr;
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);
		if (ioctl(sockfd, SIOCGIFADDR, &ifr) == -1) {
			close(sockfd);
			return 0;
		}
		struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifr.ifr_addr;
		if (inet_ntop(AF_INET, &ipaddr->sin_addr, eth0, sizeof(eth0))) {
			ips200.show_string( 0 , 0,   "eth0:");
			ips200.show_string( 0 , 16,   eth0);
		} else {
			close(sockfd);
		}
		close(sockfd);
		return 0;
}
char wlan0[INET_ADDRSTRLEN];
int show_wlan0_on_screen(){
		int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		struct ifreq ifr;
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ - 1);
		if (ioctl(sockfd, SIOCGIFADDR, &ifr) == -1) {
			close(sockfd);
			return 0;
		}
		struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifr.ifr_addr;
		if (inet_ntop(AF_INET, &ipaddr->sin_addr, wlan0, sizeof(wlan0))) {
			ips200.show_string( 0 , 32,   "wlan0:");
			ips200.show_string( 0 , 48,   wlan0);
		} else {
			close(sockfd);
		}
		close(sockfd);
		return 0;
}
int main(int,char**) {
	exec("wpa_supplicant -i wlan0 -c /etc/wpa_supplicant.conf -D nl80211");
	exec("udhcpc -i wlan0");
	ips200.init(FB_PATH,0);
	ips200.clear();
	unsigned int i;
	bool configure_state = 0;
	uint wifi_num = 1;
	while(1){
		auto network = find_wifi();
		while(!configure_state){
			find_wifi();
			ips200.show_string(0,0,"WIFI list");
			for(i = 0;i < ips_wlan_list.size();i++)ips200.show_string(0,16*(i+1),ips_wlan_list[i].c_str());
			ips200.show_string(0,16*17,"up:p16,down:p15");
			ips200.show_string(0,16*18,"press p14 to connect to:");
			if(!key_3.get_level()){
				system_delay_ms(50);
				while(!key_3.get_level());
				wifi_num--;
				ips200.clear();
			}
			if(!key_2.get_level()){
				system_delay_ms(50);
				while(!key_2.get_level());
				wifi_num++;
				ips200.clear();
			}
			if(wifi_num == 0)wifi_num = 1;
			if(wifi_num == ips_wlan_list.size()+1)wifi_num = ips_wlan_list.size();
			ips200.show_string(0,0,"WIFI list");
			for(i = 0;i < ips_wlan_list.size();i++)ips200.show_string(0,16*(i+1),ips_wlan_list[i].c_str());
			ips200.show_string(0,16*17,"up:p16,down:p15");
			ips200.show_string(0,16*18,"press p14 to connect to:");
			ips200.show_string(0,16*19,ips_wlan_list[wifi_num-1].c_str());

			if(!key_1.get_level()){
				system_delay_ms(50);
				while(!key_1.get_level());
				configure_state = 1;
			}
			if(!key_0.get_level()){
				system_delay_ms(200);
				if(!key_0.get_level()){
					ips200.clear();
					goto a1;
				}
			}
		}
		connect_wifi(network,wifi_num);
		find_wifi();
		ips200.clear();
        ips200.show_string(0,0,"WIFI list");
        for(i = 0;i < ips_wlan_list.size();i++)ips200.show_string(0,16*(i+1),ips_wlan_list[i].c_str());
        ips200.show_string(0,16*17,"up:p16,down:p15");
        ips200.show_string(0,16*18,"press p14 to connect to:");
        ips200.show_string(0,16*19,ips_wlan_list[wifi_num-1].c_str());
		configure_state = 0;
		if(!key_0.get_level()){
			system_delay_ms(200);
			if(!key_0.get_level()){
				ips200.clear();
				goto a1;
			}
		}
	}
	a1:ips200.clear();
	while(!key_0.get_level());
	while(1)
    {
        // 此处编写需要循环执行的代码
		show_eth0_on_screen();
		show_wlan0_on_screen();
		ips200.show_string(0,64,"press P13 key");   
		ips200.show_string(0,80,"for 0.2s to quit");
		if(!key_0.get_level()){
			system_delay_ms(200);
			if(!key_0.get_level()){
				ips200.clear();
				return 0;
			}
		}
		system_delay_ms(100);
    }
}

// **************************** 代码区域 ****************************

// *************************** 例程常见问题说明 ***************************
// 遇到问题时请按照以下问题检查列表检查
// 
// 问题1：终端提示未找到xxx文件
//      使用本历程，就需要使用我们逐飞科技提供的内核，否则提示xxx文件找不到
//      使用本历程，就需要使用我们逐飞科技提供的内核，否则提示xxx文件找不到
//      使用本历程，就需要使用我们逐飞科技提供的内核，否则提示xxx文件找不到
//
// 问题2：电机不转或者模块输出电压无变化
//      如果使用主板测试，主板必须要用电池供电
//      检查模块是否正确连接供电 必须使用电源线供电 不能使用杜邦线
//      查看程序是否正常烧录，是否下载报错，确认正常按下复位按键
/*********************************************************************************************************************
* LS2K0300 Opensourec Library 即（LS2K0300 开源库）是一个基于官方 SDK 接口的第三方开源库
* Copyright (c) 2022 SEEKFREE 逐飞科技
*
* 本文件是LS2K0300 开源库的一部分
*
* LS2K0300 开源库 是免费软件
* 您可以根据自由软件基金会发布的 GPL（GNU General Public License，即 GNU通用公共许可证）的条款
* 即 GPL 的第3版（即 GPL3.0）或（您选择的）任何后来的版本，重新发布和/或修改它
*
* 本开源库的发布是希望它能发挥作用，但并未对其作任何的保证
* 甚至没有隐含的适销性或适合特定用途的保证
* 更多细节请参见 GPL
*
* 您应该在收到本开源库的同时收到一份 GPL 的副本
* 如果没有，请参阅<https://www.gnu.org/licenses/>
*
* 额外注明：
* 本开源库使用 GPL3.0 开源许可证协议 以上许可申明为译文版本
* 许可申明英文版在 libraries/doc 文件夹下的 GPL3_permission_statement.txt 文件中
* 许可证副本在 libraries 文件夹下 即该文件夹下的 LICENSE 文件
* 欢迎各位使用并传播本程序 但修改内容时必须保留逐飞科技的版权声明（即本声明）
*
* 文件名称          main
* 公司名称          成都逐飞科技有限公司
* 适用平台          LS2K0300
* 店铺链接          https://seekfree.taobao.com/
*
* 修改记录
* 日期              作者           备注
* 2025-12-27        大W            first version
********************************************************************************************************************/
#include <iostream>
#include <cstring>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <regex>
#include "zf_common_headfile.hpp"

#define KEY_1_PATH        ZF_GPIO_KEY_1
#define KEY_2_PATH        ZF_GPIO_KEY_2
#define KEY_3_PATH        ZF_GPIO_KEY_3
#define KEY_4_PATH        ZF_GPIO_KEY_4

zf_driver_gpio  key_0(KEY_1_PATH, O_RDWR);
zf_driver_gpio  key_1(KEY_2_PATH, O_RDWR);
zf_driver_gpio  key_2(KEY_3_PATH, O_RDWR);
zf_driver_gpio  key_3(KEY_4_PATH, O_RDWR);

zf_device_ips200 ips200;

void sigint_handler(int signum) 
{
    printf("收到Ctrl+C，程序即将退出\n");
    exit(0);
}

void cleanup()
{
    // 需要先停止定时器线程，后面才能稳定关闭电机，电调，舵机等
    printf("程序异常退出，执行清理操作\n");
    // 关闭电机  
}
std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}
std::string get_wireless_interface() {
    // try {
    //     std::string output = exec("iwconfig 2>&1");
    //     std::istringstream iss(output);
    //     std::string line;
    //     while (getline(iss, line)) {
    //         if (line.find("IEEE 802.11") != std::string::npos) {
    //             return line.substr(0, line.find(" "));
    //         }
    //     }
    // } catch (...) {}
    return "wlan0"; // 默认值
}
struct NetworkInfo {
    std::string id;
    std::string ssid;
    std::string flags;
};
std::vector<NetworkInfo> get_networks(const std::string& interface) {
    std::vector<NetworkInfo> networks;
    std::string cmd = "wpa_cli -i " + interface + " list_networks";
    std::string output = exec(cmd.c_str());

    std::istringstream iss(output);
    std::string line;
    bool header_found = false;

    while (getline(iss, line)) {
        if (!header_found) {
            if (line.find("network id") != std::string::npos) {
                header_found = true;
            }
            continue;
        }
        std::regex re("\\t+");
        std::sregex_token_iterator it(line.begin(), line.end(), re, -1);
        std::vector<std::string> parts(it, {});

        if (parts.size() >= 3) {
            NetworkInfo info;
            info.id = parts[0];
            info.ssid = parts[1];
            info.flags = parts.size() >=4 ? parts[3] : "";
            networks.push_back(info);
        }
    }
    return networks;
}
bool select_network(const std::string& interface, const std::string& net_id) {
    std::string cmd = "wpa_cli -i " + interface + " select_network " + net_id;
    std::string result = exec(cmd.c_str());

    if (result.find("OK") != std::string::npos) {
        exec(("wpa_cli -i " + interface + " save_config").c_str());
        //cout << "已成功选择网络，正在连接..." << endl;
		exec("udhcpc -i wlan0");
        // 释放和获取IP地址
        system(("dhclient -r " + interface + " >/dev/null 2>&1").c_str());
        system(("dhclient " + interface + " >/dev/null 2>&1").c_str());
        return true;
    }
    return false;
}
std::vector<std::string> ips_wlan_list;
std::string interface = get_wireless_interface();
auto find_wifi(){
	ips_wlan_list.clear();
	auto networks = get_networks(interface);
	for (size_t i = 0; i < networks.size(); ++i) {
		std::string status = (networks[i].flags.find("CURRENT") != std::string::npos) ?
					   "当前连接" : "已保存";
		if(status == "当前连接")ips_wlan_list.push_back(std::to_string(i+1)+"."+networks[i].ssid+"(connect)");
		else ips_wlan_list.push_back(std::to_string(i+1)+"."+networks[i].ssid);
	}
	return networks;
}
void connect_wifi(std::vector<NetworkInfo> networks,int choice){
	if (select_network(interface, networks[choice-1].id)) {
		std::string status = exec(("wpa_cli -i " + interface + " status").c_str());
	}
}
char eth0[INET_ADDRSTRLEN];
int show_eth0_on_screen(){
		int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		struct ifreq ifr;
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);
		if (ioctl(sockfd, SIOCGIFADDR, &ifr) == -1) {
			close(sockfd);
			return 0;
		}
		struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifr.ifr_addr;
		if (inet_ntop(AF_INET, &ipaddr->sin_addr, eth0, sizeof(eth0))) {
			ips200.show_string( 0 , 0,   "eth0:");
			ips200.show_string( 0 , 16,   eth0);
		} else {
			close(sockfd);
		}
		close(sockfd);
		return 0;
}
char wlan0[INET_ADDRSTRLEN];
int show_wlan0_on_screen(){
		int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		struct ifreq ifr;
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ - 1);
		if (ioctl(sockfd, SIOCGIFADDR, &ifr) == -1) {
			close(sockfd);
			return 0;
		}
		struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifr.ifr_addr;
		if (inet_ntop(AF_INET, &ipaddr->sin_addr, wlan0, sizeof(wlan0))) {
			ips200.show_string( 0 , 32,   "wlan0:");
			ips200.show_string( 0 , 48,   wlan0);
		} else {
			close(sockfd);
		}
		close(sockfd);
		return 0;
}
int main(int,char**) {
	exec("wpa_supplicant -i wlan0 -c /etc/wpa_supplicant.conf -D nl80211");
	exec("udhcpc -i wlan0");
	ips200.init(FB_PATH,0);
	ips200.clear();
	unsigned int i;
	bool configure_state = 0;
	uint wifi_num = 1;
	while(1){
		auto network = find_wifi();
		while(!configure_state){
			find_wifi();
			ips200.show_string(0,0,"WIFI list");
			for(i = 0;i < ips_wlan_list.size();i++)ips200.show_string(0,16*(i+1),ips_wlan_list[i].c_str());
			ips200.show_string(0,16*17,"up:p16,down:p15");
			ips200.show_string(0,16*18,"press p14 to connect to:");
			if(!key_3.get_level()){
				system_delay_ms(50);
				while(!key_3.get_level());
				wifi_num--;
				ips200.clear();
			}
			if(!key_2.get_level()){
				system_delay_ms(50);
				while(!key_2.get_level());
				wifi_num++;
				ips200.clear();
			}
			if(wifi_num == 0)wifi_num = 1;
			if(wifi_num == ips_wlan_list.size()+1)wifi_num = ips_wlan_list.size();
			ips200.show_string(0,0,"WIFI list");
			for(i = 0;i < ips_wlan_list.size();i++)ips200.show_string(0,16*(i+1),ips_wlan_list[i].c_str());
			ips200.show_string(0,16*17,"up:p16,down:p15");
			ips200.show_string(0,16*18,"press p14 to connect to:");
			ips200.show_string(0,16*19,ips_wlan_list[wifi_num-1].c_str());

			if(!key_1.get_level()){
				system_delay_ms(50);
				while(!key_1.get_level());
				configure_state = 1;
			}
			if(!key_0.get_level()){
				system_delay_ms(200);
				if(!key_0.get_level()){
					ips200.clear();
					goto a1;
				}
			}
		}
		connect_wifi(network,wifi_num);
		find_wifi();
		ips200.clear();
        ips200.show_string(0,0,"WIFI list");
        for(i = 0;i < ips_wlan_list.size();i++)ips200.show_string(0,16*(i+1),ips_wlan_list[i].c_str());
        ips200.show_string(0,16*17,"up:p16,down:p15");
        ips200.show_string(0,16*18,"press p14 to connect to:");
        ips200.show_string(0,16*19,ips_wlan_list[wifi_num-1].c_str());
		configure_state = 0;
		if(!key_0.get_level()){
			system_delay_ms(200);
			if(!key_0.get_level()){
				ips200.clear();
				goto a1;
			}
		}
	}
	a1:ips200.clear();
	while(!key_0.get_level());
	while(1)
    {
        // 此处编写需要循环执行的代码
		show_eth0_on_screen();
		show_wlan0_on_screen();
		ips200.show_string(0,64,"press P13 key");   
		ips200.show_string(0,80,"for 0.2s to quit");
		if(!key_0.get_level()){
			system_delay_ms(200);
			if(!key_0.get_level()){
				ips200.clear();
				return 0;
			}
		}
		system_delay_ms(100);
    }
}

// **************************** 代码区域 ****************************

// *************************** 例程常见问题说明 ***************************
// 遇到问题时请按照以下问题检查列表检查
// 
// 问题1：终端提示未找到xxx文件
//      使用本历程，就需要使用我们逐飞科技提供的内核，否则提示xxx文件找不到
//      使用本历程，就需要使用我们逐飞科技提供的内核，否则提示xxx文件找不到
//      使用本历程，就需要使用我们逐飞科技提供的内核，否则提示xxx文件找不到
//
// 问题2：电机不转或者模块输出电压无变化
//      如果使用主板测试，主板必须要用电池供电
//      检查模块是否正确连接供电 必须使用电源线供电 不能使用杜邦线
//      查看程序是否正常烧录，是否下载报错，确认正常按下复位按键
