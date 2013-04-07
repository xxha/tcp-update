#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <string.h>  
#include <signal.h>  
#include <errno.h>
#include <wait.h>

#define	MEMSIZE	1024
      
/************************************************* 
* Function    :  
* Description : 服务器对客户端的处理 
* Calls       :  
* Called By   :  
* Input       :  
* Output      :  
* Return      :  
*************************************************/  
void process_conn_server(int s)  
{  
	ssize_t size = 0;  
        char buffer[MEMSIZE];          /* 数据的缓冲区 */ 
	char cmd[MEMSIZE];
	int ret = 0;
	int status = 0;
	int temp = 0;
      
        for (;;)  
        {                           /* 循环处理过程 */  
            /* 从套接字中读取数据放到缓冲区buffer中 */  
		memset(buffer, '\0', MEMSIZE);  
		size = read(s, buffer, MEMSIZE);  
		if (size == 0)  
		{                       /* 没有数据 */  
			return;  
		}  
//		printf("CMD: %s\n", buffer);
//		buffer[size-1] = ' ';
		sprintf(cmd, "%s > /dev/null 2>&1", buffer);
		status = system(cmd);
		if(status == -1){
			temp = 0x51;
			ret = write(s, &temp, sizeof(int));
			if(ret <= 0) printf("write failed\n");
		}else{
			if (WIFEXITED(status)){
				temp = WEXITSTATUS(status);
				ret = write(s, &temp, sizeof(int));	
	                        if(ret <= 0) printf("write failed\n");
			}else{
				temp = 0x50;
				ret = write(s, &temp, sizeof(int));
                	        if(ret <= 0) printf("write failed\n");
			}
		}
        } 
}  
      
/************************************************* 
* Function    :  
* Description : 客户端的处理过程 
* Calls       :  
* Called By   :  
* Input       :  
* Output      :  
* Return      :  
*************************************************/  
void process_conn_client(int s)  
{  
        ssize_t size = 0;  
        char buffer[MEMSIZE];          /* 数据的缓冲区 */  
	int ret = 0;
      
        for (;;)  
        {                           /* 循环处理过程 */  
		memset(buffer, '\0', MEMSIZE);
            /* 从标准输入中读取数据放到缓冲区buffer中 */  
		size = read(0, buffer, MEMSIZE);
		if (size > 1)  
		{                       /* 读到数据 */  
			ret = write(s, buffer, size-1); /* 发送给服务器 */ 
			if(ret < 0){
				perror("socket write");
			}else{
				memset(buffer, '\0', MEMSIZE);
				size = read(s, buffer, MEMSIZE);   /* 从服务器读取数据 */  
				printf("Server send: 0x%x to me\n", *((int *)buffer));
			}
		}  
        }  
}  
      
/************************************************* 
* Function    :  
* Description :  
* Calls       :  
* Called By   : tcp_server.c tcp_client.c 
* Input       :  
* Output      :  
* Return      :  
*************************************************/  
void sig_proccess(int signo)  
{  
        printf("Catch a exit signal\n");  
        exit(0);  
}  

void sig_child(int signo)
{
	pid_t pid;
	int   stat;

	while( (pid=waitpid(-1,&stat,WNOHANG))>0)
	printf("child %d terminated\n",pid);

	return;
}

      
/************************************************* 
* Function    :  
* Description :  
* Calls       :  
* Called By   : tcp_server.c tcp_client.c 
* Input       :  
* Output      :  
* Return      :  
*************************************************/  
void sig_pipe(int sign)  
{  
        printf("Catch a SIGPIPE signal\n");  
	signal(SIGPIPE, sig_pipe);
}  

