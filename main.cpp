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
* 2025-02-27        大W            first version
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
#include "zf_common_headfile.h"

#define KEY_0       "/dev/zf_driver_gpio_key_0"
#define KEY_1       "/dev/zf_driver_gpio_key_1"
#define KEY_2       "/dev/zf_driver_gpio_key_2"
#define KEY_3       "/dev/zf_driver_gpio_key_3"

int16_t data_index = 0;
using namespace std;

// 执行命令并获取输出
void ips200_list_string(uint8 start_line,uint8 line,const char dat[]){
    ips200_show_string(0,16*(line+start_line-2),dat);
}
//ips200_list_string(从第几行开始显示，显示第几行，你要输出的玩意);
string exec(const char* cmd) {
    char buffer[128];
    string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw runtime_error("popen() failed!");
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

// 获取无线接口
string get_wireless_interface() {
    try {
        string output = exec("iwconfig 2>&1");
        istringstream iss(output);
        string line;
        while (getline(iss, line)) {
            if (line.find("IEEE 802.11") != string::npos) {
                return line.substr(0, line.find(" "));
            }
        }
    } catch (...) {}
    return "wlan0"; // 默认值
}

// 网络信息结构体
struct NetworkInfo {
    string id;
    string ssid;
    string flags;
};

// 获取并解析网络列表
vector<NetworkInfo> get_networks(const string& interface) {
    vector<NetworkInfo> networks;
    string cmd = "wpa_cli -i " + interface + " list_networks";
    string output = exec(cmd.c_str());

    istringstream iss(output);
    string line;
    bool header_found = false;

    while (getline(iss, line)) {
        // 改进的标题检测逻辑
        if (!header_found) {
            if (line.find("network id") != string::npos) {
                header_found = true;
            }
            continue;
        }

        // 改进的字段解析逻辑
        regex re("\\t+");
        sregex_token_iterator it(line.begin(), line.end(), re, -1);
        vector<string> parts(it, {});

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

// 选择指定网络
bool select_network(const string& interface, const string& net_id) {
    string cmd = "wpa_cli -i " + interface + " select_network " + net_id;
    string result = exec(cmd.c_str());

    if (result.find("OK") != string::npos) {
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
vector<string> ips_wlan_list;
string interface = get_wireless_interface();
auto find_wifi(){
	ips_wlan_list.clear();
	// 获取无线接口
	//cout << "当前无线接口: " << interface << endl;

	// 获取网络列表
	auto networks = get_networks(interface);
	//cout <<networks.empty()<<endl;
	if (networks.empty()) {
		//cerr << "没有找到已保存的网络" << endl;
		//return 1;
	}

	// 显示网络列表
	//cout << "\n已保存的WiFi网络:" << endl;
	for (size_t i = 0; i < networks.size(); ++i) {
		string status = (networks[i].flags.find("CURRENT") != string::npos) ?
					   "当前连接" : "已保存";
		//cout << i+1 << ". " << networks[i].ssid << " (" << status << ")" << endl;
		if(status == "当前连接")ips_wlan_list.push_back(to_string(i+1)+"."+networks[i].ssid+"(connect)");
		else ips_wlan_list.push_back(to_string(i+1)+"."+networks[i].ssid);
	}
	return networks;
}
void connect_wifi(vector<NetworkInfo> networks,int choice){
	// 用户输入处理
	//cout << "\n请输入要连接的网络编号: ";
	// int choice;
	// cin >> choice;

	if (cin.fail() || choice < 1 || choice > static_cast<int>(networks.size())) {
		//cerr << "无效的输入" << endl;
		//return 1;
	}

	// 执行网络切换
	if (select_network(interface, networks[choice-1].id)) {
		//cout << "\n连接状态:" << endl;
		string status = exec(("wpa_cli -i " + interface + " status").c_str());
		//cout << status;
	} else {
		//cerr << "网络切换失败" << endl;
		//return 1;
	}
}
char eth0[INET_ADDRSTRLEN];
int show_eth0_on_screen(){
	    // 创建socket
		int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sockfd < 0) {
			//std::cerr << "无法创建socket" << std::endl;
		}
	
		struct ifreq ifr;
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);
	
		// 获取IPv4地址
		if (ioctl(sockfd, SIOCGIFADDR, &ifr) == -1) {
			//std::cerr << "无法获取IP地址（请检查接口名称和权限）" << std::endl;
			close(sockfd);
			return 0;
		}
	
		// 转换二进制地址为字符串
		struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifr.ifr_addr;
		if (inet_ntop(AF_INET, &ipaddr->sin_addr, eth0, sizeof(eth0))) {
			//std::cout << "eth0 IP地址: " << eth0 << std::endl;
			ips200_show_string( 0 , 0,   "eth0:");
			ips200_show_string( 0 , 16,   eth0);
		} else {
			//std::cerr << "地址转换失败" << std::endl;
			close(sockfd);
		}
		
		close(sockfd);
		return 0;
}
char wlan0[INET_ADDRSTRLEN];
int show_wlan0_on_screen(){
	    // 创建socket
		int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sockfd < 0) {
			//std::cerr << "无法创建socket" << std::endl;
		}
	
		struct ifreq ifr;
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ - 1);
	
		// 获取IPv4地址
		if (ioctl(sockfd, SIOCGIFADDR, &ifr) == -1) {
			//std::cerr << "无法获取IP地址（请检查接口名称和权限）" << std::endl;
			close(sockfd);
			return 0;
		}
	
		// 转换二进制地址为字符串
		struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifr.ifr_addr;
		if (inet_ntop(AF_INET, &ipaddr->sin_addr, wlan0, sizeof(wlan0))) {
			//std::cout << "wlan0 IP地址: " << wlan0 << std::endl;
			ips200_show_string( 0 , 32,   "wlan0:");
			ips200_show_string( 0 , 48,   wlan0);
		} else {
			//std::cerr << "地址转换失败" << std::endl;
			close(sockfd);
		}
	
		close(sockfd);
		return 0;
}
int main() {
	exec("wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant.conf");
	ips200_init("/dev/fb0");
	ips200_clear();
	unsigned int i;
	//vector<char> wlanlist_temp;
	bool configure_state = 0;
	uint wifi_num = 1;
	while(1){
		auto network = find_wifi();
		while(!configure_state){
			find_wifi();
			ips200_list_string(1,1,"WIFI list");
			for(i = 0;i < ips_wlan_list.size();i++)		ips200_list_string(2,i+1,ips_wlan_list[i].c_str());
			ips200_list_string(18,1,"up:p16,down:p15");
			ips200_list_string(19,1,"press p14 to connect to:");
			if(!gpio_get_level(KEY_3)){
				system_delay_ms(50);
				while(!gpio_get_level(KEY_3));
				//if(gpio_get_level(KEY_3)){
					wifi_num--;
					ips200_clear();
				//}
			}
			if(!gpio_get_level(KEY_2)){
				system_delay_ms(50);
				while(!gpio_get_level(KEY_2));
				//if(gpio_get_level(KEY_2)){
					wifi_num++;
					ips200_clear();
				//}
			}
			//cout<<wifi_num<<endl;
			if(wifi_num == 0)wifi_num = 1;
			if(wifi_num == ips_wlan_list.size()+1)wifi_num = ips_wlan_list.size();
			ips200_list_string(1,1,"WIFI list");
			for(i = 0;i < ips_wlan_list.size();i++)		ips200_list_string(2,i+1,ips_wlan_list[i].c_str());
			ips200_list_string(18,1,"up:p16,down:p15,quit:p13");
			ips200_list_string(19,1,"press p14 to connect to:");
			ips200_list_string(20,1,ips_wlan_list[wifi_num-1].c_str());

			if(!gpio_get_level(KEY_1)){
				system_delay_ms(50);
				while(!gpio_get_level(KEY_1));
				//if(gpio_get_level(KEY_1)){
					configure_state = 1;
				//}
			}
			if(!gpio_get_level(KEY_0)){
				system_delay_ms(200);
				if(!gpio_get_level(KEY_0)){
					//ips200_show_string(0,96,"quit");
					ips200_clear();
					goto a1;
				}
			}
		}
		connect_wifi(network,wifi_num);
		find_wifi();
		ips200_clear();
		ips200_list_string(1,1,"WIFI list");
		for(i = 0;i < ips_wlan_list.size();i++)		ips200_list_string(2,i+1,ips_wlan_list[i].c_str());
		ips200_list_string(18,1,"up:p16,down:p15,quit:p13");
		ips200_list_string(19,1,"press p14 to connect to:");
		ips200_list_string(20,1,ips_wlan_list[wifi_num-1].c_str());
		configure_state = 0;
		if(!gpio_get_level(KEY_0)){
			system_delay_ms(200);
			if(!gpio_get_level(KEY_0)){
				//ips200_show_string(0,96,"quit");
				ips200_clear();
				goto a1;
			}
		}
	}
	a1:ips200_clear();
	while(!gpio_get_level(KEY_0));
	while(1)
    {
        // 此处编写需要循环执行的代码
		show_eth0_on_screen();
		show_wlan0_on_screen();
		ips200_show_string(0,64,"press P13 key");
		ips200_show_string(0,80,"for 0.2s to quit");
		if(!gpio_get_level(KEY_0)){
			system_delay_ms(200);
			if(!gpio_get_level(KEY_0)){
				//ips200_show_string(0,96,"quit");
				ips200_clear();
				return 0;
			}
		}
		system_delay_ms(100);
    }
}