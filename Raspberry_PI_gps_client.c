/*
1. 전원을 켜고 시스템에 로그인하십시오. 터미널을 열고 다음 명령을 입력하여 GPS 모듈 용 패키지를 설치할 수 있습니다.
sudo apt-get update && sudo apt-get -y install gpsd gpsd-clients python-gps
2. gpsd 서비스를 시작하고 제어하십시오.
사용 : 
sudo systemctl enable gpsd.socket

시작 : 
sudo systemctl start gpsd.socket

다시 시작 : 
sudo systemctl restart gpsd.socket

상태 확인 : 
sudo systemctl status gpsd.socket

4. / etc / default / gpsd에서 gpsd 구성 파일
수정 / dev 폴더의 직렬 포트 이름에 따라 "DEVICE"매개 변수를 수정하십시오.
USB 케이블을 통해 Raspberry Pi에 연결하면 일반적으로 "/ dev / ttyUSB0"으로 이름이 지정됩니다.
"nano"또는 "vim.tiny"편집기를 사용하여 완료 할 수 있습니다.

grep -v "#" /etc/default/gpsd |grep -v "^$"


서비스 재시작 :
sudo systemctl restart gpsd.socket

마지막으로이 명령을 사용하여 GPS 모듈에서 정보를 얻습니다.
sudo cgps -s


sudo gpsd /dev/ttyUSB0 -F /var/run/gpsd.sock

sudo nano /boot/cmdline.txt


sudo nano /boot/config.txt

gcc -o gps_client gps_client.c -lm -lgps -lpthread

sudo ./gps_client 34.224.82.208 33333 asd
*/

#include <gps.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
	
#define BUF_SIZE 300
#define NAME_SIZE 20
	
void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);
	
char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];
	
int rc;
struct timeval tv;
int main(int argc, char *argv[])
{
	
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void * thread_return;
	if(argc!=4) {
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	 }
	
	sprintf(name, "[%s]", argv[3]);
	sock=socket(PF_INET, SOCK_STREAM, 0);
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));
	  
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
		error_handling("connect() error");
	
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);
	close(sock);  
	
	
	return 0;
}
	
void * send_msg(void * arg)   // send thread main
{
	int sock=*((int*)arg);
	char name_msg[NAME_SIZE+BUF_SIZE];
	//////////////////////////////////////////////gps code
	

	struct gps_data_t gps_data;
	if ((rc = gps_open("localhost", "2947", &gps_data)) == -1) {
	printf("code: %d, reason: %s\n", rc, gps_errstr(rc));
	//return EXIT_FAILURE;
	return 0;
	}
	
	gps_stream(&gps_data, WATCH_ENABLE | WATCH_JSON, NULL);
	//////////////////////////////////////////////gps code
	while(1) 
	{
		 
    /* wait for 2 seconds to receive data */
    if (gps_waiting (&gps_data, 1000000)) {
        /* read data */
        if ((rc = gps_read(&gps_data)) == -1) {
            printf("error occured reading gps data. code: %d, reason: %s\n", rc, gps_errstr(rc));
        } else {
            /* Display data from the GPS receiver. */
            if ((gps_data.status == STATUS_FIX) && 
                (gps_data.fix.mode == MODE_2D || gps_data.fix.mode == MODE_3D) &&
                !isnan(gps_data.fix.latitude) && 
                !isnan(gps_data.fix.longitude)) {
                    //gettimeofday(&tv, NULL); EDIT: tv.tv_sec isn't actually the timestamp!
                    printf("latitude: %f, longitude: %f, speed: %f, timestamp: %lf\n", gps_data.fix.latitude, gps_data.fix.longitude, gps_data.fix.speed, gps_data.fix.time); //EDIT: Replaced tv.tv_sec with gps_data.fix.time
            } else {
                printf("no GPS data available\n");
            }
        }
    }

    sleep(3);



		//fgets(msg, BUF_SIZE, stdin);
		//if(!strcmp(msg,"q\n")||!strcmp(msg,"Q\n")) 
		//{
		//	close(sock);
		//	exit(0);
		//}
		//sprintf(name_msg,"%s %s", name, msg);
		//sprintf(msg, "latitude: %f, longitude: %f, speed: %f, timestamp: %lf\n",gps_data.fix.latitude, gps_data.fix.longitude, gps_data.fix.speed, gps_data.fix.time);
		sprintf(msg, "%f, %f\n",gps_data.fix.latitude, gps_data.fix.longitude);
		
		write(sock, msg, strlen(msg));
		
	}
	gps_stream(&gps_data, WATCH_DISABLE, NULL);
gps_close (&gps_data);

return EXIT_SUCCESS;
	return NULL;
}
	
void * recv_msg(void * arg)   // read thread main
{
	int sock=*((int*)arg);
	char name_msg[NAME_SIZE+BUF_SIZE];
	int str_len;
	
	while(1)
	{
		str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);
		if(str_len==-1) 
			return (void*)-1;
		name_msg[str_len]=0;
		fputs(name_msg, stdout);
	}
	return NULL;
}
	
void error_handling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
