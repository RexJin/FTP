#include "head.h"

int set_conf()
{
	conf cf;
	bzero(&cf,sizeof(cf));
	strcpy(cf.ip_addr,"192.168.111.132");
	cf.port=1234;
	cf.child_num=5;
	int fd;
	fd=open(CONFIG,O_WRONLY|O_CREAT,0666);
	write(fd,&cf,sizeof(conf));
	close(fd);
}
int conf_init(conf* pconf)
{
	int fd;
	fd=open(CONFIG,O_RDONLY);
	if(-1==fd)
	{
		perror("open");
		return -1;
	}
	read(fd,pconf,sizeof(conf));
	close(fd);
}
int make_child(pchild cptr,int n)
{
	int i;
	int fds[2];
	int ret;
	int pid;
	for(i=0;i<n;i++)
	{
		ret=socketpair(AF_LOCAL,SOCK_STREAM,0,fds);	//在创建子进程前初始化 使子进程也可以拿到 
		if(-1==ret)
		{
			perror("socketpair");
			return -1;
		}
		pid=fork();
		if(pid==0)//子进程 
		{
			close(fds[1]);
			child_handle(fds[0]);
		}
		close(fds[0]);
		cptr[i].pid=pid;//存储子进程ID 
		cptr[i].fds=fds[1];
		cptr[i].busy=0;
	}
	return 0;
}

int child_handle(int fds)//子进程执行
{
	int fd;
	int flag=1;
	while(1)
	{
		recv_fd(fds,&fd);//若父进程未发送描述符，则阻塞
		child_mission(fd);//子进程业务逻辑
		write(fds,&flag,4);//向父进程发送完成信号
	}
	return 0;
}

int child_mission(int sfd)
{
	int ret;
	int len;
	char cmd[128];
	if(!check_pwd(sfd))
	{
		return 0;
	}

	while(1)
	{
		bzero(cmd,sizeof(cmd));
		ret=recv(sfd,cmd,sizeof(cmd),0);
		if(-1==ret)
		{
			perror("recv cmd");
			close(sfd);
			return -1;
		}	
		if(0==ret)//客户端关闭或断开
		{
			close(sfd);
			return 0;
		}	
		if(strcmp(cmd,"cd")==0)
		{
			op_cd(sfd);
		}
		else if(strcmp(cmd,"ls")==0)
		{
			op_ls(sfd);
		}
		else if(strcmp(cmd,"puts")==0)
		{
			op_puts(sfd);
		}
		else if(strcmp(cmd,"gets")==0)
		{
			op_gets(sfd);
		}
		else if(strcmp(cmd,"remove")==0)
		{	
			op_remove(sfd);
		}
		else if(strcmp(cmd,"pwd")==0)
		{
			op_pwd(sfd);
		}
		else
		{
			close(sfd);
			break;
		}
	}
	return 0;
}

int check_pwd(int sfd)
{
	int flag;
	char name[128]={0};
	char pwd[128]={0};
	struct spwd *sp;//系统存储用户名密码的结构体
	char salt[512]={0};
	recv(sfd,name,sizeof(name),0);
	recv(sfd,pwd,sizeof(pwd),0);
	if((sp=getspnam(name))==NULL)//通过用户名获取密码
	{
		flag=0;
		send(sfd,&flag,4,0);
		return 0;
	}
	get_salt(salt,sp->sp_pwdp);//获取salt值
	if(strcmp(sp->sp_pwdp,crypt(pwd,salt))==0)//用户密码经过crypt与系统存储密码比较
	{
		flag=1;
		send(sfd,&flag,4,0);
		return 1;
	}
	else
	{
		flag=0;
		send(sfd,&flag,4,0);
		return 0;
	}
}

int get_salt(char* salt,char* passwd)
{
	int i,j;
	for(i=0,j=0;passwd[i]&&j!=3;++i)
	{
		if(passwd[i]=='$')
			++j;
	}
	strncpy(salt,passwd,i-1);
	return 0;
}
int op_ls(int sfd)
{
	send_dir(sfd);//发送当前目录信息
	return 0;
}

int op_cd(int sfd)
{
	int ret;
	data_t buf;
	char cur_path[128];
	bzero(cur_path,sizeof(cur_path));
	bzero(&buf,sizeof(buf));
	ret=recv(sfd,&buf.len,4,0);
	if(-1==ret)
	{
		perror("op_cd recv");
		return -1;
	}
	ret=recv(sfd,buf.buf,buf.len,0);
	if(-1==ret)
	{
		perror("op_cd recv2");
		return -1;
	}
	getcwd(cur_path,sizeof(cur_path));
	ret=chdir(buf.buf);//切换目录
	if(-1==ret)
	{
		perror("op_cd chdir");
		return -1;
	}
	getcwd(buf.buf,sizeof(buf.buf));
	if(strcmp(buf.buf,"/home")==0||strcmp(buf.buf,"/")==0)//控制访问范围
	{
		chdir(cur_path);
		op_pwd(sfd);
	}
	else
	{
		op_pwd(sfd);
	}
	return 0;
}

int op_pwd(int sfd)
{
	int ret;
	data_t buf;
	bzero(&buf,sizeof(buf));
	getcwd(buf.buf,sizeof(buf.buf));//获取当前目录
	buf.len=strlen(buf.buf);
	ret=send(sfd,&buf,buf.len+4,0);
	if(-1==ret)
	{
		perror("op_pwd send1");
		return -1;
	}
	int flag=1;
	bzero(&buf,sizeof(buf));
	buf.len=sizeof(int);
	memcpy(buf.buf,&flag,4);
	send(sfd,&buf,buf.len+4,0);//发送结束标志，告诉客户端当前文件传输结束
	if(-1==ret)
	{
		perror("op_pwd send2");
		return -1;
	}
	return 0;
}

int op_gets(int sfd)
{
	int ret;
	int pos;
	data_t buf;
	bzero(&buf,sizeof(buf));
	ret=recv(sfd,&pos,4,0);//读取续传位置
	if(-1==ret)
	{
		perror("op_gets recv1");
		return -1;
	}
	ret=recv(sfd,&buf.len,4,0);//读取文件名长度
	if(-1==ret)
	{
		perror("op_gets recv2");
		return -1;
	}
	ret=recv(sfd,buf.buf,buf.len,0);//读取文件名
	if(-1==ret)
	{
		perror("op_gets recv3");
		return -1;
	}
	send_file(sfd,buf.buf,pos);
	return 0;
}

int op_puts(int sfd)
{
	int ret;
	data_t buf;
	bzero(&buf,sizeof(buf));
	recv(sfd,&buf.len,4,0);
	if(-1==ret)
	{
		perror("op_puts recv1");
		return -1;
	}
	recv(sfd,buf.buf,buf.len,0);//读取文件名
	if(-1==ret)
	{
		perror("op_puts recv1");
		return -1;
	}
	recv_file(sfd);
	return 0;
}

int op_remove(int sfd)
{
	int ret;
	data_t buf;
	bzero(&buf,sizeof(buf));
	recv(sfd,&buf.len,4,0);
	if(-1==ret)
	{
		perror("op_remove recv1");
		return -1;
	}
	recv(sfd,buf.buf,buf.len,0);//读取文件名
	if(-1==ret)
	{
		perror("op_remove recv1");
		return -1;
	}
	remove(buf.buf);
	return 0;
}

int send_dir(int sfd)
{
	char path[100];
	bzero(path,sizeof(path));
	getcwd(path,sizeof(path));//获取当前目录
	DIR* dir;
	dir=opendir(path);
	if(NULL==dir)
	{	
		perror("opendir");
		return -1;
	}
	struct dirent* pt;
	data_t buf;
	struct stat st1;
	while(pt=readdir(dir))
	{
		if(!strcmp(pt->d_name,".")||!strcmp(pt->d_name,".."))
		{}
		else
		{
			bzero(&buf,sizeof(buf));
			stat(pt->d_name,&st1);
			sprintf(buf.buf,"%-20s%ldB",pt->d_name,st1.st_size);
			buf.len=strlen(buf.buf);
			send(sfd,&buf,buf.len+4,0);
		}
	}	
	int flag=1;
	bzero(&buf,sizeof(buf));
	buf.len=sizeof(int);
	memcpy(buf.buf,&flag,4);
	send(sfd,&buf,buf.len+4,0);//发送结束标志，告诉客户端当前文件传输结束
	closedir(dir);
	return 0;
}

int send_file(int sfd,char* path,int pos)//子进程给客户端发送文件
{
	int ret;
	data_t buf;
	buf.len=strlen(path);
	strcpy(buf.buf,path);
	send(sfd,&buf,buf.len+4,0);//发送文件名(通过len控制发送长度。4为len值)
	if(-1==ret)
	{
		perror("send_file send");
		return -1;
	}
	int fd=open(path,O_RDONLY);
	if(-1==fd)
	{
		perror("send_file open");
		return -1;
	}
	lseek(fd,pos,SEEK_SET);//定位到发送位置
	while(bzero(&buf,sizeof(buf)),(buf.len=read(fd,buf.buf,sizeof(buf.buf)))>0)
	{
		send_n(sfd,(char*)&buf,buf.len+4);//每一轮循环发送完毕
	}
	bzero(&buf,sizeof(buf));
	int flag=1;
	buf.len=sizeof(int);
	memcpy(buf.buf,&flag,4);
	send(sfd,&buf,buf.len+4,0);//发送结束标志，告诉客户端当前文件传输结束
	return 0;
}

int recv_file(int sfd)
{
	int ret;
	int len;
	data_t buf;
	bzero(&buf,sizeof(buf));
	ret=recv(sfd,&buf.len,4,0);//读取buf.len值
	if(-1==ret)
	{
		perror("recv_file recv1");
		return -1;
	}
	recv(sfd,buf.buf,buf.len,0);//读取文件名
	int fd;
	fd=open(buf.buf,O_RDWR|O_CREAT,0666);//根据传过来的文件名创建同名文件
	if(-1==fd)
	{
		perror("recv_file open");
		return -1;
	}
	int flag=1;
	while(1)//循环读取并写入文件
	{
		bzero(&buf,sizeof(buf));
		recv_n(sfd,(char*)&buf.len,4);//读取数据长度
		recv_n(sfd,buf.buf,buf.len);//读取数据
		if(buf.len==4 && !memcmp(buf.buf,&flag,4))//判断是否文件结束标志
		{
			break;
		}
		ret=write(fd,buf.buf,buf.len);//写入文件
		if(ret<0)
		{
			perror("write");
			return -1;
		}
	}
}
