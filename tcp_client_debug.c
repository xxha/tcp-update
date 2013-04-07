#include <stdio.h>  
#include <stdlib.h>  
#include <strings.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <unistd.h>  
#include <linux/in.h>  
#include <signal.h>  
      
extern void sig_proccess(int signo);  
extern void sig_pipe(int signo);  
static int s;  

void sig_proccess_client(int signo)  
{  
//	printf("Catch a exit signal\n");  
	close(s);  
	exit(0);  
}  
      
/************************************************* 
* Function    :  
* Description : 客户端主程序 
* Calls       : process_conn_client 
* Called By   :  
* Input       :  
* Output      :  
* Return      :  
*************************************************/  
int main(int argc, char *argv[])  
{  
        int port;  
        struct sockaddr_in server_addr; /* 服务器地址结构 */  
	int ret = 0;
      
        signal(SIGINT, sig_proccess);  
        signal(SIGPIPE, sig_pipe);  
      
        /* 建立一个流式套接字 */  
        s = socket(AF_INET, SOCK_STREAM, 0);  
        if (s < 0)  
        {                           /* 出错 */  
            printf("socket error\n");  
            return -1;  
        }  
      
        port = atoi(argv[2]);  
      
        /* 设置服务器地址 */  
        bzero(&server_addr, sizeof(server_addr));   /* 清0 */  
        server_addr.sin_family = AF_INET;   /* 协议族 */  
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);    /* 本地地址 */  
        server_addr.sin_port = htons(port); /* 服务器端口 */  
      
	inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

        /* 连接服务器 */  
        ret = connect(s, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));  
	if(ret < 0){
		printf("Connect to server %s failed!\n", argv[1]);
		exit(1);
	}
        process_conn_client(s);     /* 客户端处理过程 */  
        close(s);                   /* 关闭连接 */  
}  

