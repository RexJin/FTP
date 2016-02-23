#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
typedef struct tdata{//文件数据传输
	int len;
	char buf[1000];
}data_t,*pdata_t;

int check_pwd(int);//用户名密码检测
int op_ls(int);//ls命令处理
int op_cd(int);//cd命令处理
int op_pwd(int);//pwd命令处理
int op_gets(int);//gets命令处理,下载
int op_puts(int);//puts命令处理,上传
int op_remove(int);//remove命令处理
int recv_msg(int);//接收信息
int recv_file(int,int);//接收文件
int send_file(int,char*);//发送文件
void send_n(int,char*,int);//循环发送
void recv_n(int,char*,int);//循环接收
