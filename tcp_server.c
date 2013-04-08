#include <stdio.h>  
#include <stdlib.h>  
#include <strings.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <unistd.h>  
#include <linux/in.h>  
#include <signal.h>  
      
extern void sig_proccess(int signo);  
extern void sig_child(int signo);
     
#define BACKLOG 5               /* 侦听队列长度 */  
 
void sig_hup(int signo)
{
        printf("No HUP!!!\n");
}
     
/************************************************* 
* Function    :  
* Description : 服务端主程序 
* Calls       : process_conn_server 
* Called By   :  
* Input       :  
* Output      :  
* Return      :  
*************************************************/  
int main(int argc, char *argv[])  
{ 
 
        if (argc != 2)  
        {  
            printf("Usage:%s port_number\n", argv[0]);  
            return 1;  
        }  
      
        int port;  
        int ss, sc;                 /* ss为服务器的socket描述符,sc为客户端的socket描述符 */  
        struct sockaddr_in server_addr; /* 服务器地址结构 */  
        struct sockaddr_in client_addr; /* 客户端地址结构 */  
        int err;                    /* 返回值 */  
        pid_t pid;                  /* 分叉的进行id */  
      
        signal(SIGINT, sig_proccess);  
        signal(SIGPIPE, sig_proccess);  
	signal(SIGCHLD, sig_child);
//	signal(SIGHUP, sig_hup);

      
        /* 建立一个流式套接字 */  
        ss = socket(AF_INET, SOCK_STREAM, 0);  
        if (ss < 0)  
        {                           /* 出错 */  
            printf("socket error\n");  
            return -1;  
        }  
        //获得输入的端口   
        port = atoi(argv[1]);  
      
        /* 设置服务器地址 */  
        bzero(&server_addr, sizeof(server_addr));   /* 清0 */  
        server_addr.sin_family = AF_INET;   /* 协议族 */  
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);    /* 本地地址 */  
        server_addr.sin_port = htons(port); /* 服务器端口 */  
      
        /* 绑定地址结构到套接字描述符 */  
        err = bind(ss, (struct sockaddr *)&server_addr, sizeof(server_addr));  
        if (err < 0)  
        {                           /* 出错 */  
            printf("bind error\n");  
            return -1;  
        }  

	for (;;){      
        	/* 设置侦听 */  
        	err = listen(ss, BACKLOG);  
        	if (err < 0)  
        	{                           /* 出错 */  
        		printf("listen error\n");  
			return -1;  
        	}  
      
            	int addrlen = sizeof(struct sockaddr);  
            	/* 接收客户端连接 */  
            	sc = accept(ss, (struct sockaddr *)&client_addr, &addrlen);  
            	if (sc < 0)  
            	{                       /* 出错 */  
			printf("Can't accept the new connection!\n");
                	continue;           /* 结束本次循环 */  
            	}  
      
            	/* 建立一个新的进程处理到来的连接 */  
            	pid = fork();           /* 分叉进程 */  
            	if (pid == 0)  
            	{                       /* 子进程中 */  
                	close(ss);          /* 在子进程中关闭服务器的侦听 */  
                	process_conn_server(sc);    /* 处理连接 */  
            	}else{  
                	close(sc);          /* 在父进程中关闭客户端的连接 */  
            	}  
        }  
}  

