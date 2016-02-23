#include "func.h"

int send_cmd(int sfd)
{
	int len;
	char cmd[128];
	if(!check_pwd(sfd))
	{
		return 0;
	}

	while(1)
	{
		bzero(cmd,sizeof(cmd));
		printf("input:");
		fscanf(stdin,"%s",cmd);
		send(sfd,cmd,strlen(cmd),0);
		system("clear");
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
		else if(strcmp(cmd,"pwd")==0)
		{
			op_pwd(sfd);
		}
		else if(strcmp(cmd,"remove")==0)
		{
			op_remove(sfd);
		}
		else
		{
			printf("wrong command.\n");
		}
	}
	return 0;
}

int check_pwd(int sfd)
{
	
	int flag;
	char name[128]={0};
	char pwd[128]={0};
	printf("username:");
	fscanf(stdin,"%s",name);
	send(sfd,name,strlen(name),0);
	printf("password:");
	fscanf(stdin,"%s",pwd);
	send(sfd,pwd,strlen(pwd),0);
	recv(sfd,&flag,4,0);
	if(flag==0)
		printf("wrong username or password!\n");
	return flag;	
}

int op_ls(int sfd)
{
	recv_msg(sfd);
	return 0;
}


int op_cd(int sfd)
{
	int ret;
	data_t buf;
	bzero(&buf,sizeof(buf));
	fscanf(stdin,"%s",buf.buf);//读取标准输入
	buf.len=strlen(buf.buf);
	ret=send(sfd,&buf,buf.len+4,0);
	if(-1==ret)
	{
		perror("send cd");
		return -1;
	}
	recv_msg(sfd);
	return 0;
}

int op_gets(int sfd)
{
	int fd;
	int pos=0;//发送读取位置(用于断点续传)
	struct stat statbuf;
	data_t buf;
	bzero(&buf,sizeof(buf));
	fscanf(stdin,"%s",buf.buf);
	buf.len=strlen(buf.buf);
	if(access(buf.buf,0)==0)//文件已经存在(可访问),断点续传
	{
		fd=open(buf.buf,O_RDONLY);
		fstat(fd,&statbuf);
		pos=statbuf.st_size;
		close(fd);
	}
	send(sfd,&pos,4,0);//发送断点位置
	send(sfd,&buf,buf.len+4,0);//发送要下载的文件名
	recv_file(sfd,pos);
	printf("download success.\n");
}

int op_puts(int sfd)
{
	data_t buf;
	bzero(&buf,sizeof(buf));
	fscanf(stdin,"%s",buf.buf);
	buf.len=strlen(buf.buf);
	send(sfd,&buf,buf.len+4,0);//发送要上传的文件名
	send_file(sfd,buf.buf);
	printf("upload success.\n");
}

int op_pwd(int sfd)
{
	recv_msg(sfd);
}

int op_remove(int sfd)
{
	data_t buf;
	bzero(&buf,sizeof(buf));
	fscanf(stdin,"%s",buf.buf);
	buf.len=strlen(buf.buf);
	send(sfd,&buf,buf.len+4,0);//发送要删除的文件名
	printf("remove success.\n");//此处应加判断，后补
}

int recv_msg(int sfd)//接收和打印信息
{
	int ret;
	int len;
	int flag=1;
	data_t buf;
	while(1)
	{
		bzero(&buf,sizeof(buf));
		ret=recv(sfd,&buf.len,4,0);//接收数据长度信息
		if(-1==ret)
		{
			perror("recv1");
			return -1;
		}
		ret=recv(sfd,buf.buf,buf.len,0);//接收数据
		if(-1==ret)
		{
			perror("recv2");
			return -1;
		}
		if(buf.len==4&&!memcmp(buf.buf,&flag,4))//判断结束
		{
			break;
		}
		printf("%s\n",buf.buf);
	}
}

int recv_file(int sfd,int pos)
{
	int ret;
	int len;
	data_t buf;
	bzero(&buf,sizeof(buf));
	ret=recv(sfd,&buf.len,4,0);//读取buf.len值
	if(-1==ret)
	{
		perror("recv1");
		return -1;
	}
	recv(sfd,buf.buf,buf.len,0);//读取文件名
	int fd;
	fd=open(buf.buf,O_RDWR|O_CREAT,0666);//根据传过来的文件名创建同名文件
	if(-1==fd)
	{
		perror("open");
		return -1;
	}
	int flag=1;
	int cur_pos;
	lseek(fd,pos,SEEK_SET);//定位续传位置
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
		cur_pos=lseek(fd,0,SEEK_CUR);
		printf("downloading...%d\n",cur_pos);
	}
}

int send_file(int sfd,char* path)//子进程给客户端发送文件
{
	int ret;
	data_t buf;
	buf.len=strlen(path);
	strcpy(buf.buf,path);
	send(sfd,&buf,buf.len+4,0);//发送文件名(通过len控制发送长度。4为len值)
	if(-1==ret)
	{
		perror("send1");
		return -1;
	}
	int fd=open(path,O_RDONLY);
	if(-1==fd)
	{
		perror("open");
		return -1;
	}
	while(bzero(&buf,sizeof(buf)),(buf.len=read(fd,buf.buf,sizeof(buf.buf)))>0)
	{
		send_n(sfd,(char*)&buf,buf.len+4);//每一轮循环发送完毕
	}
	bzero(&buf,sizeof(buf));
	int flag=1;
	buf.len=sizeof(int);
	memcpy(buf.buf,&flag,4);
	send(sfd,&buf,buf.len+4,0);//发送结束标志，告诉客户端当前文件传输结束
	return;
}
