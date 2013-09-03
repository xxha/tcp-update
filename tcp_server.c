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
     
#define BACKLOG 5               /* listen queue length */  
 
void sig_hup(int signo)
{
        printf("No HUP!!!\n");
}
     
/************************************************* 
* Function    :  
* Description : server main routine
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
        int ss, sc;                 /* ss: server socket descriptor, sc: client socket descriptor */  
        struct sockaddr_in server_addr; /* sever address struct */  
        struct sockaddr_in client_addr; /* client address struct */  
        int err;                    /* return value */  
        pid_t pid;                  /* fork create sub process */  
      
        signal(SIGINT, sig_proccess);  
        signal(SIGPIPE, sig_proccess);  
	signal(SIGCHLD, sig_child);
	//signal(SIGHUP, sig_hup);

      
        /* setup stream socket */  
        ss = socket(AF_INET, SOCK_STREAM, 0);  
        if (ss < 0)  
        {   /* error */  
            printf("socket error\n");  
            return -1;  
        }  
        /* get input port */  
        port = atoi(argv[1]);  
      
        /* config server address */  
        bzero(&server_addr, sizeof(server_addr));   /* clear 0 */  
        server_addr.sin_family = AF_INET;   /* protocol set */  
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);    /* local address */  
        server_addr.sin_port = htons(port); /* server port */  
      
        /* bind address struct to socket descriptor */  
        err = bind(ss, (struct sockaddr *)&server_addr, sizeof(server_addr));  
        if (err < 0)  
        {  
            printf("bind error\n");  
            return -1;  
        }  

	for (;;){      
        	/* set listen */  
        	err = listen(ss, BACKLOG);  
        	if (err < 0)  
        	{
        		printf("listen error\n");  
			return -1;  
        	}  
      
            	int addrlen = sizeof(struct sockaddr);  
            	/* accpet client connect */  
            	sc = accept(ss, (struct sockaddr *)&client_addr, &addrlen);  
            	if (sc < 0)  
            	{	/* error */
			printf("Can't accept the new connection!\n");
                	continue;           /* current loop over */  
            	}  
      
            	/* create a new process to deal with the comming connection */  
            	pid = fork();           /* create sub process */  
            	if (pid == 0)  
            	{                       /* in sub process */  
                	close(ss);          /* close server listen */  
                	process_conn_server(sc);    /* process connection */  
            	}else{  
                	close(sc);          /* in parent process, close client connection. */  
            	}  
        }  
}  

