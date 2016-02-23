#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <shadow.h>
#include <crypt.h>
#define CONFIG "..//conf//config"
typedef struct config
{
	char ip_addr[32];
	int port;
	int child_num;
}conf;

typedef struct pro_child{//子进程信息(父进程维护)
	pid_t pid;//子进程ID
	int fds;//socketpair父进程一端，用于接收子进程发送的信息
	int busy;//进程忙碌标志
}child,*pchild;

typedef struct tdata{//数据传输载体
	int len;//buf信息长度(变长效果)
	char buf[1000];
}data_t,*pdata_t;

int set_conf();//建立配置文件
int config_init(conf*);//读取配置文件
int make_child(pchild,int);//创建子进程
int child_handle(int);//子进程执行函数
int child_mission(int);//子进程业务逻辑

int check_pwd(int);//检测用户名密码
int get_salt(char*,char*);//获取密码的salt值
int op_ls(int);//处理ls请求
int op_cd(int);//处理cd请求
int op_pwd(int);//处理pwd请求
int op_puts(int);//处理puts上传请求
int op_gets(int);//处理gets下载请求
int op_remove(int);//处理remove请求
int send_dir(int);//发送目录信息(ls)
int send_file(int,char*,int);//子进程发送文件
int recv_file(int);//子进程接收上传的文件
void send_n(int ,char*,int);//循环发送
void recv_n(int ,char*,int);//循环接收

void send_fd(int ,int);//传送文件描述符
void recv_fd(int ,int*);//接收文件描述符

