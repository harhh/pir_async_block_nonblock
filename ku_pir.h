struct ku_pir_data {
// 센서에 저장되는 timestamp
long unsigned int timestamp;
// 센서의 RISING, FALLING여부 ( 0 : RISING, 1: FALLING)
int rf_flag;
};

#define FALLING 0
#define RISING 1

#define KUPIR_MAX_MSG 10
#define KUPIR_SENSOR 17
#define DEV_NAME "ku_pir_dev"

#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2
#define IOCTL_NUM3 IOCTL_START_NUM+3
#define IOCTL_NUM4 IOCTL_START_NUM+4

#define IOCTL_NUM 'z'
#define KU_NONBLOCKING _IOWR(IOCTL_NUM, IOCTL_NUM1, unsigned long *)
#define KU_BLOCKING _IOWR(IOCTL_NUM, IOCTL_NUM2, unsigned long *)
#define KU_ASYNCHRONOUS _IOWR(IOCTL_NUM, IOCTL_NUM3, unsigned long *)
#define KU_INSERT _IOWR(IOCTL_NUM, IOCTL_NUM4, unsigned long *)
