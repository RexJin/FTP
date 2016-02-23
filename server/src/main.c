#include "head.h"

int main(int argc,char* argv[])
{
	conf cf;
//	set_conf();
	conf_init(&cf);//读取配置文件
	printf("ip = %s,port = %d,child_num = %d\n",cf.ip_addr,cf.port,cf.child_num);
	pchild cptr;//子进程序列 
	int num=cf.child_num;//子进程数目 
	cptr=(pchild)calloc(num,sizeof(child));//申请子进程结构体序列空间 
	make_child(cptr,num);//创建子进程 
	
	int sfd=socket(AF_INET,SOCK_STREAM,0);
	if(-1==sfd)
	{
		perror("socket");
		return -1;
	}
	struct sockaddr_in ser;
	bzero(&ser,sizeof(ser));
	ser.sin_family=AF_INET;
	ser.sin_port=htons(cf.port);
	ser.sin_addr.s_addr=inet_addr(cf.ip_addr);
	int ret;	
	ret=bind(sfd,(struct sockaddr *)&ser,sizeof(struct sockaddr));
	if(-1==ret)
	{
		perror("bind");
		return -1;
	}
	listen(sfd,num);
	int epfd;
	epfd=epoll_create(1);
	if(-1==epfd)
	{
		perror("epoll_create");
		return -1;
	}
	struct epoll_event ev,*evs; 
	evs=(struct epoll_event*)calloc(num+1,sizeof(struct epoll_event));
	
	ev.events=EPOLLIN;
	ev.data.fd=sfd;
	ret=epoll_ctl(epfd,EPOLL_CTL_ADD,sfd,&ev);	
	if(-1==ret)
	{
		perror("epoll_ctl");
		return -1;
	}
	int i;
	for(i=0;i<num;i++)
	{
		ev.events=EPOLLIN;
		ev.data.fd=cptr[i].fds;//使用了EPOLLIN，写的时候epoll不会触发 
		ret=epoll_ctl(epfd,EPOLL_CTL_ADD,cptr[i].fds,&ev);//监控子进程是否发送结束信号	
		if(-1==ret)
		{
			perror("epoll_ctl1");
			return -1;
		}
	}
	int j;
	int flag;
	int size;
	int new_fd;
	int connected=0;
	while(1)
	{
		ret=epoll_wait(epfd,evs,num+1,-1);	
		if(ret>0)
		{
			for(i=0;i<ret;i++)
			{
				if(evs[i].data.fd == sfd && evs[i].events == EPOLLIN)
				{
					new_fd=accept(sfd,NULL,NULL);
					if(-1==new_fd)
					{
						perror("accept");	
						return -1;
					}
					for(j=0;j<num;j++)
					{
						if(cptr[j].busy == 0)//有空闲子进程 
						{
							break;
						}
					}
					if(j !=num)
					{
						send_fd(cptr[j].fds,new_fd);//将new_fd通过socketpair写端传送给子进程 
						cptr[j].busy=1;//设置为忙碌
						connected++;
						printf("new client connect. tatal connected = %d\n",connected);
						close(new_fd);//关闭主进程的new_fd
					}
				}
				for(j=0;j<num;j++)//若有子进程发送数据(实际可为任意数据),表示任务结束,不再忙碌
				{
					if(evs[i].data.fd == cptr[j].fds && evs[i].events == EPOLLIN)
					{
						
						read(cptr[j].fds,&flag,4);//读取flag(防止不停水平触发)
						cptr[j].busy=0;
						connected--;
						printf("one client disconnect. tatal connected = %d\n",connected);
					}
				}
			}
		}	
	}
	return 0;	
}	
	
