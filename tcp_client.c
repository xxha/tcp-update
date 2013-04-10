#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define	SD_ERROR		0x10
#define	SD_FORMAT_ERROR		0x11
#define	TMP_WRITE_ERROR		0x12
#define	MTD_MOUNT_ERROR 	0x13
#define	MBR_BACKUP_ERROR	0x14
#define	SD_MOUNT_ERROR		0x15
#define	IMAGE_GET_ERROR		0x16
#define	IMAGE_UNZIP_ERROR	0x17
#define	IMAGE_DEL_ERROR		0x18
#define	MD5_ERROR		0x19
#define CD_TMP_ERROR		0x20
#define WRITE_USR_ERROR		0x21
#define CHANGE_MODE_ERROR	0x22
#define KILL_USR_ERROR		0x23
#define UMOUNT_USR_ERROR	0x24
#define GET_BOARD_ERROR		0x25
#define RM_FILE_ERROR		0x26
#define WRITE_KERNEL_ERROR	0x27
#define WRITE_ROOTFS_ERROR	0x28
#define SLEEP_ERROR		0x29
#define INSMOD_ERROR		0x30
#define RMMOD_ERROR		0x31

#define	MEMSIZE	1024

#define BOARD_NAME_MAX_SIZE 10
#define REDUNDANT 13

extern void sig_proccess(int signo);
extern void sig_pipe(int signo);
static int mod_s;
static char boardname[BOARD_NAME_MAX_SIZE];

FILE * log_fd = NULL;

void sig_proccess_client(int signo)
{
	//printf("Catch a exit signal\n");
	close(mod_s);
	exit(0);
}

int sendcmd(int s, char * cmdp, unsigned int len)
{
	int ret = 0;
	char buffer[MEMSIZE];

	ret = write(s, cmdp, len); /* send to server */
	if(ret < 0){
		perror("socket write");
		return -1;
	}else{
		memset(buffer, '\0', MEMSIZE);
		read(s, buffer, MEMSIZE);   /* read data from server */
		//printf("Server send: 0x%x to me\n", *((int *)buffer));

		return *((int *)buffer);
	}
}

void sdup(int s, char * hostip, char * filename, char * board)
{
	ssize_t size = 0;
	char cmd[MEMSIZE];
	int ret = 0;

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "mdev -s");
	fprintf(log_fd, "%s ", cmd);
	sendcmd(s, cmd, sizeof(cmd));

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "ls /dev/mmcblk0*");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret < 0){
		fprintf(log_fd,"No SD card found, upgrade SD card exit.\n");
		fclose(log_fd);
		printf("No SD card found, upgrade SD card exit.\n");
		exit(SD_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	sleep(1);

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "umount /mnt/sd");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		sleep(1);
		ret = sendcmd(s, cmd, sizeof(cmd));
		if(ret != 0) fprintf(log_fd,"FAILED\n");
	}else{
		fprintf(log_fd, "OK.\n");
	}

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "/tmp/parted.sh /dev/mmcblk0");
	fprintf(log_fd, "%s ", cmd);
	sendcmd(s, cmd, sizeof(cmd));

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "mke2fs -T ext2 /dev/mmcblk0p1");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd, "FAILED.\n");
		memset(cmd, '\0', MEMSIZE);
		sprintf(cmd, "/tmp/mke2fs -T ext2 /dev/mmcblk0p1");
		fprintf(log_fd, "%s ", cmd);
		ret = sendcmd(s, cmd, sizeof(cmd));
		if(ret != 0){
			fprintf(log_fd,"FAILED\n");
			fclose(log_fd);
			printf("Make ext2 sd card failed.\n");
			exit(SD_FORMAT_ERROR);
		}
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "mkdir /tmp/images");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd, "FAILED.\n");
	}else{
		fprintf(log_fd, "OK.\n");
	}

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "mount -t jffs2 /dev/mtdblock3 /tmp/images");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd, "FAILED.\n");
	}else{
		fprintf(log_fd, "OK.\n");
	}

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "dd if=/dev/mmcblk0 of=/tmp/images/mbr.bin bs=512 count=1");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Backup sd card mbr failed.\n");
		exit(MBR_BACKUP_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "mount / -o remount,rw");
	fprintf(log_fd, "%s ", cmd);
	sendcmd(s, cmd, sizeof(cmd));

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "mkdir /mnt/sd");
	fprintf(log_fd, "%s ", cmd);
	sendcmd(s, cmd, sizeof(cmd));

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "mount -t ext2 /dev/mmcblk0p1 /mnt/sd");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Mount sd card failed.\n");
		exit(SD_MOUNT_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "ftpget -u v400 -p v400 %s /mnt/sd/%s /usr/local/%s/share/%s", hostip, filename, board, filename);
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Get %s failed.\n", filename);
		exit(IMAGE_GET_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "tar -xzvf /mnt/sd/%s -C /mnt/sd", filename);
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Unzip %s failed.\n", filename);
		exit(IMAGE_UNZIP_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "rm -rf /mnt/sd/%s", filename);
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Delete image failed.\n");
		exit(IMAGE_DEL_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "md5sum -c /mnt/sd/sd.md5");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Check md5 failed.\n");
		exit(MD5_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "umount /mnt/sd");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd,"FAILED\n");
	}else{
		fprintf(log_fd, "OK.\n");
	}

	fprintf(log_fd, "SD upgrade SUCCESS\n");
	printf("SD upgrade SUCCESS\r");
	fflush(stdout);
}

void getboard(char * filename, char * board)
{
        int i;
        char *tmp;

        if(board == NULL || filename == NULL \
			|| strlen(filename) < BOARD_NAME_MAX_SIZE)
                exit(GET_BOARD_ERROR);

        tmp = board;
        for(i = 0; *(filename + REDUNDANT + i) != '.' \
			&& i < BOARD_NAME_MAX_SIZE; i++ ){
                *tmp++ = *(filename + REDUNDANT + i);
        }
}

void flashup(int s, char * hostip, char * filename, char *board)
{
	char cmd[MEMSIZE];
	int ret = 0;

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "cd /tmp");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("cd tmp failed.\n");
		exit(CD_TMP_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "ftpget -u v400 -p v400 %s kill-mods kill-mods", hostip);
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Remote get kill-mods failed.\n");
		exit(IMAGE_GET_ERROR);
	}
	fprintf(log_fd, "OK.\n");


	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "chmod +x /tmp/kill-mods");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("chmod kill-mods failed.\n");
		exit(CHANGE_MODE_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "/tmp/kill-mods");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret < 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Execute kill-mods failed.\n");
		exit(KILL_USR_ERROR);
	}
	fprintf(log_fd, "OK.\n");


	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "rm -rf /tmp/kill-mods");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret < 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Remove kill-mods failed.\n");
		exit(RM_FILE_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "sleep 1");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret < 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Sleep 1 failed.\n");
		exit(SLEEP_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "ftpget -u v400 -p v400 %s veextmp.ko veextmp.ko", hostip);
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret < 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Remote get veextmp failed.\n");
		exit(IMAGE_GET_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "insmod veextmp.ko");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret < 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("insmod veextmp failed.\n");
		exit(INSMOD_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "rmmod veextmp.ko");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret < 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("rmmod veextmp failed.\n");
		exit(RMMOD_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "umount /usr");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret < 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("umount /usr failed.\n");
		exit(UMOUNT_USR_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "ftpget -u v400 -p v400 %s %s /usr/local/%s/share/%s", hostip, filename, board, filename);
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Remote get %s failed.\n", filename);
		exit(IMAGE_GET_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "sleep 1");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret < 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Sleep 1 failed.\n");
		exit(SLEEP_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "tar xvf %s", filename);
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Unzip %s failed.\n", filename);
		exit(IMAGE_UNZIP_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "sleep 2");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret < 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Sleep 2 failed.\n");
		exit(SLEEP_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "rm -rf %s", filename);
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Remove %s failed.\n", filename);
		exit(RM_FILE_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "ls version");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret == 0){
		fprintf(log_fd,"OK\n");

		memset(cmd, '\0', MEMSIZE);
		sprintf(cmd, "diff version /etc/version");
		fprintf(log_fd, "%s ", cmd);
		ret = sendcmd(s, cmd, sizeof(cmd));
		fprintf(log_fd, "OK.\n");
		if(ret != 0){
			fprintf(log_fd,"Rootfs version is different, upgrade kernel and rootfs\n");
			printf("Upgrading kernel and rootfs.\r");
			fflush(stdout);

			memset(cmd, '\0', MEMSIZE);
			sprintf(cmd, "yes|uuu -w kernel.img Kernel");
			fprintf(log_fd, "%s ", cmd);
			ret = sendcmd(s, cmd, sizeof(cmd));
			if(ret != 0){
				fprintf(log_fd,"FAILED\n");
				fclose(log_fd);
				printf("Write kernel failed.\n");
				exit(WRITE_KERNEL_ERROR);
			}
			fprintf(log_fd, "OK.\n");

			memset(cmd, '\0', MEMSIZE);
			sprintf(cmd, "rm -rf kernel.img tmp");
			fprintf(log_fd, "%s ", cmd);
			ret = sendcmd(s, cmd, sizeof(cmd));
			if(ret != 0){
				fprintf(log_fd,"FAILED\n");
				fclose(log_fd);
				printf("Remove kernel failed.\n");
				exit(RM_FILE_ERROR);
			}
			fprintf(log_fd, "OK.\n");

			memset(cmd, '\0', MEMSIZE);
			sprintf(cmd, "yes|uuu -w rootfs.img RootFS");
			fprintf(log_fd, "%s ", cmd);
			ret = sendcmd(s, cmd, sizeof(cmd));
			if(ret != 0){
				fprintf(log_fd,"FAILED\n");
				fclose(log_fd);
				printf("Write rootfs failed.\n");
				exit(WRITE_ROOTFS_ERROR);
			}
			fprintf(log_fd, "OK.\n");

			memset(cmd, '\0', MEMSIZE);
			sprintf(cmd, "rm -rf rootfs.img tmp kroot.md5 version");
			fprintf(log_fd, "%s ", cmd);
			ret = sendcmd(s, cmd, sizeof(cmd));
			if(ret != 0){
				fprintf(log_fd,"FAILED\n");
				fclose(log_fd);
				printf("Remove rootfs kroot.md5 version failed.\n");
				exit(RM_FILE_ERROR);
			}
			fprintf(log_fd, "OK.\n");
		} else {
			fprintf(log_fd, "Rootfs version is the same, don't need upgrade!\n");

			memset(cmd, '\0', MEMSIZE);
			sprintf(cmd, "rm -rf kernel.img rootfs.img kroot.md5 version");
			fprintf(log_fd, "%s ", cmd);
			ret = sendcmd(s, cmd, sizeof(cmd));
			if(ret != 0){
				fprintf(log_fd,"FAILED\n");
				fclose(log_fd);
				printf("Remove kernel rootfs kroot.md5 version failed.\n");
				exit(RM_FILE_ERROR);
			}
			fprintf(log_fd, "OK.\n");
		}
	} else {
		fprintf(log_fd, "FAILED\n");
		fprintf(log_fd, "No version file, don't need upgrade!\n");

		memset(cmd, '\0', MEMSIZE);
		sprintf(cmd, "rm -rf kernel.img rootfs.img kroot.md5 version");
		fprintf(log_fd, "%s ", cmd);
		ret = sendcmd(s, cmd, sizeof(cmd));
		if(ret != 0){
			fprintf(log_fd,"FAILED\n");
			fclose(log_fd);
			printf("Remove kernel rootfs kroot.md5 version failed.\n");
			exit(RM_FILE_ERROR);
		}
		fprintf(log_fd, "OK.\n");
	}

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "sleep 1");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Sleep 1 failed.\n");
		exit(SLEEP_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	printf("Upgrading usr applications.\r");
	fflush(stdout);

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "yes|uuu -w userfs.jffs2 UserFSA");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("write usr image failed.\n");
		exit(WRITE_USR_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	memset(cmd, '\0', MEMSIZE);
	sprintf(cmd, "rm tmp userfs.jffs2 userfs.jffs2.md5");
	fprintf(log_fd, "%s ", cmd);
	ret = sendcmd(s, cmd, sizeof(cmd));
	if(ret != 0){
		fprintf(log_fd,"FAILED\n");
		fclose(log_fd);
		printf("Remove usr image and md5 failed.\n");
		exit(RM_FILE_ERROR);
	}
	fprintf(log_fd, "OK.\n");

	fprintf(log_fd, "Norflash upgrade SUCCESS\n");
	printf("Norflash upgrade SUCCESS\r");
	fflush(stdout);
}

int main(int argc, char *argv[])
{
	int port;
	struct sockaddr_in server_addr; /* server address struct */
	int ret = 0;
	char *sdimage = "ux400-module-sd.tar.gz";

	log_fd = fopen(argv[6], "w+");
	if(log_fd == NULL){
		printf("Log file %s open failed.\n", argv[6]);
		exit(4);
	}

	fprintf(log_fd, "Start...\n");

	if(argc != 7){
		printf("Usage: target_IP port_num source_IP image_file_name boardname log_file_name\n");
		fprintf(log_fd, "Wrong input, argc=%d\n", argc);
		fclose(log_fd);
		exit(1);
	}

	signal(SIGINT, sig_proccess);
	signal(SIGPIPE, sig_pipe);

	/* setup a stream socket */
	mod_s = socket(AF_INET, SOCK_STREAM, 0);
	if (mod_s < 0)
	{	/* error */
		printf("socket error\n");
		fprintf(log_fd, "socket error\n");
		fclose(log_fd);
		exit(2);
	}

	port = atoi(argv[2]);

	/* config server address */
	bzero(&server_addr, sizeof(server_addr));   /* clear 0 */
	server_addr.sin_family = AF_INET;   /* protocol set */
	server_addr.sin_port = htons(port); /* server port */
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);

	bzero(&(server_addr.sin_zero), 8);

	/* connect server */
	ret = connect(mod_s, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if(ret < 0){
		printf("Connect to server %s failed.\n", argv[1]);
		fprintf(log_fd, "Connect to server %s failed, port is %d, error is : %s\n", argv[1], port, strerror(errno));
		fclose(log_fd);
		exit(4);
	}
#if 0
	process_conn_client(s);     /* client process */
#endif
	fprintf(log_fd, "Server %s connect successed, start upgrading now\n", argv[1]);

	if (strcmp(sdimage, argv[4]) == 0) {
		fprintf(log_fd, "Upgrading SD card.\n");
		printf("Upgrading SD card.\r");
		fflush(stdout);
		sdup(mod_s, argv[3], argv[4], argv[5]);
	} else {
		fprintf(log_fd, "Upgrading Norflash.\n", argv[1]);
		printf("Upgrading Norflash.\r");
		fflush(stdout);
		flashup(mod_s, argv[3], argv[4], argv[5]);
	}

	close(mod_s);		  /* close connect */
	fclose(log_fd);
	exit(0);
}

