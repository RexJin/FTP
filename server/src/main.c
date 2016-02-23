#include "head.h"

int main(int argc,char* argv[])
{
	conf cf;
//	set_conf();
	conf_init(&cf);//��ȡ�����ļ�
	printf("ip = %s,port = %d,child_num = %d\n",cf.ip_addr,cf.port,cf.child_num);
	pchild cptr;//�ӽ������� 
	int num=cf.child_num;//�ӽ�����Ŀ 
	cptr=(pchild)calloc(num,sizeof(child));//�����ӽ��̽ṹ�����пռ� 
	make_child(cptr,num);//�����ӽ��� 
	
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
		ev.data.fd=cptr[i].fds;//ʹ����EPOLLIN��д��ʱ��epoll���ᴥ�� 
		ret=epoll_ctl(epfd,EPOLL_CTL_ADD,cptr[i].fds,&ev);//����ӽ����Ƿ��ͽ����ź�	
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
						if(cptr[j].busy == 0)//�п����ӽ��� 
						{
							break;
						}
					}
					if(j !=num)
					{
						send_fd(cptr[j].fds,new_fd);//��new_fdͨ��socketpairд�˴��͸��ӽ��� 
						cptr[j].busy=1;//����Ϊæµ
						connected++;
						printf("new client connect. tatal connected = %d\n",connected);
						close(new_fd);//�ر������̵�new_fd
					}
				}
				for(j=0;j<num;j++)//�����ӽ��̷�������(ʵ�ʿ�Ϊ��������),��ʾ�������,����æµ
				{
					if(evs[i].data.fd == cptr[j].fds && evs[i].events == EPOLLIN)
					{
						
						read(cptr[j].fds,&flag,4);//��ȡflag(��ֹ��ͣˮƽ����)
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
	
