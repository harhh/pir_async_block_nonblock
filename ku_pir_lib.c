#include <stdio.h>

#include <sys/fcntl.h>
#include <sys/ioctl.h>

#include <signal.h>
#include <stdlib.h>
#include "ku_pir.h"

#define KUPIR_DEV "/dev/ku_pir_dev"

int ku_pir_open();
int ku_pir_close(int fd);
struct ku_pir_data* ku_pir_nonblocking(int fd, long unsigned int ts);
struct ku_pir_data* ku_pir_blocking(int fd, long unsigned int ts);
struct ku_pir_data* ku_pir_asynchronous(int fd, long unsigned int ts);
int ku_pir_insertData(int fd, long unsigned int ts, int rf_flag);
void sig_handler(int signo, siginfo_t* info, void* context);

int ku_pir_open()
{
	int fd = open(KUPIR_DEV, O_RDWR);

	if (fd == -1)
		return 0;
	else
		return fd;
}

int ku_pir_close(int fd)
{
	return close(fd) != 0;
}

struct ku_pir_data* ku_pir_nonblocking(int fd, long unsigned int ts)
{
	struct ku_pir_data *data = (struct ku_pir_data*)malloc(sizeof(struct ku_pir_data));
	
	data->timestamp = ts;

	if(ioctl(fd, KU_NONBLOCKING, data)<0){
		free(data);
		return NULL;
	}
	else	return data;

}

struct ku_pir_data* ku_pir_blocking(int fd, long unsigned int ts)
{
	struct ku_pir_data *data = (struct ku_pir_data*)malloc(sizeof(struct ku_pir_data));

	data->timestamp = ts;

	ioctl(fd, KU_BLOCKING, data);
	return data;
}

struct ku_pir_data* ku_pir_asynchronous(int fd, long unsigned int ts)
{
	struct ku_pir_data *data = (struct ku_pir_data*)malloc(sizeof(struct ku_pir_data));

	data->timestamp = ts;
	if(ioctl(fd, KU_ASYNCHRONOUS, data)<0){
		free(data);
		return NULL;
	}
	else	return data;
}

int ku_pir_insertData(int fd, long unsigned int ts, int rf_flag)
{
	struct ku_pir_data *insert_data = (struct ku_pir_data*)malloc(sizeof(struct ku_pir_data));

	insert_data->timestamp = ts;
	insert_data->rf_flag = rf_flag;
	
	if (ioctl(fd, KU_INSERT, insert_data)== 0) {
		insert_data = 0;
		printf("insert success\n");
		return 1;
	} else {
		insert_data = 0;
		printf("insert fail\n");
		return 0;
	}
}


void sig_handler(int signo, siginfo_t* info, void* context)
{
	printf("ASYNC RESULT :%d", info->si_value.sival_int);
	exit(0);	
}
