
#define _PRINT_IO_LATENCY
#define _GNU_SOURCE

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/file.h>
#include<sys/types.h>
#include<pthread.h>
#include<assert.h>
#include<string.h>
#include<time.h>
#include<aio.h>

#define BUFFER_SIZE 1048576

short devno;
unsigned long offset;
int length;
char rwType;
double reqtime;
//time_t timeStamp;

int fd;
char * buf;
int ct;

void aio_completion_handler(sigval_t sigval)
{
    //用来获取读aiocb结构的指针
    struct aiocb *pcb = (struct aiocb *)sigval.sival_ptr;
#ifdef _PRINT_IO_LATENCY
    struct timeval te;
	gettimeofday(&te, NULL);
	time_t ts = *((time_t *)(pcb->aio_sigevent.sigev_notify_attributes));
	time_t latency = 1000000 * te.tv_sec + te.tv_usec - ts;
	printf("%u, %u\n", ts, latency);
#endif
}

void emit_io() {
}

int main(int argc, char **argv) {

	fd = open("/dev/rbd0", O_RDWR|O_DIRECT);
	posix_memalign((void **)&buf, 512, BUFFER_SIZE);
	memset(buf, 'c', BUFFER_SIZE);

	initialize("Cambridge/prxy_1.trace");

	struct timeval start;
	int i;
	for(i = 0; i < 10000000; ++i) {
		nextrequest(&devno, &offset, &length, &rwType, &reqtime);
		gettimeofday(&start, NULL);
		time_t * ts = (time_t *)malloc(sizeof(time_t)); //new time_t();
		//time_t timeStamp 
		*ts = 1000000 * start.tv_sec + start.tv_usec;

		struct aiocb my_aiocb;
		bzero(&my_aiocb, sizeof(my_aiocb));		
		my_aiocb.aio_fildes = fd;
		my_aiocb.aio_buf = buf;
		my_aiocb.aio_nbytes = length;
		my_aiocb.aio_offset = offset;
		//填充aiocb中有关回调通知的结构体sigevent
		my_aiocb.aio_sigevent.sigev_notify = SIGEV_THREAD;//使用线程回调通知
		my_aiocb.aio_sigevent.sigev_notify_function = aio_completion_handler;//设置回调函数
		my_aiocb.aio_sigevent.sigev_notify_attributes = (void*)(ts);//NULL;//使用默认属性
		my_aiocb.aio_sigevent.sigev_value.sival_ptr = &my_aiocb;//在aiocb控制块中加入自己的引用

		aio_write(&my_aiocb);

		//发得过快，栈溢出，SE fault
		usleep(100000);
	}

	sleep(1);

	finalize();
	free(buf);
	close(fd);

	return 0;
}
