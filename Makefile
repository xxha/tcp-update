CC = gcc
LD = ld
CROSS_COMPILE = /home/xxha/cross_compile/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin/arm-none-linux-gnueabi-
all:	client server  
          
client:	tcp_process.o tcp_client.o  
	$(CC) -o client tcp_process.o tcp_client.o  

server:	tcp_process.o tcp_server.o
	$(CROSS_COMPILE)$(CC) tcp_process.o tcp_server.o -o server
#tcp_process.o:tcp_process.c
#	$(CROSS_COMPILE)$(CC) -c tcp_process.c
#tcp_server.o: tcp_server.c
#	$(CROSS_COMPILE)$(CC) -c tcp_server.c
clean:  
	rm -f *.o  

