
#define _PRINT_IO_LATENCY
#define _GNU_SOURCE

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<assert.h>
#include<errno.h>
#include<string.h>
#include<sys/file.h>
#include<sys/types.h>
#include<aio.h>
#include<unistd.h>

#define BUFFER_SIZE 1048576

int fd;
char * buf;

void aio_completion_handler(sigval_t sigval)
{
    //用来获取读aiocb结构的指针
    struct aiocb *pcb;
    int ret;

    pcb = (struct aiocb *)sigval.sival_ptr;

    printf("hello\n");

    //判断请求是否成功
    if(aio_error(pcb) == 0)
    {
        //获取返回值
        ret = aio_return(pcb);
        printf("读返回值为:%d\n",ret);
    }
}

int main(int argc,char **argv)
{
    int ret;
    struct aiocb my_aiocb;

    fd = open("test.txt", O_RDWR|O_DIRECT);

	posix_memalign((void **)&buf, 512, BUFFER_SIZE);

    //填充aiocb的基本内容
    bzero(&my_aiocb, sizeof(my_aiocb));

    my_aiocb.aio_fildes = fd;
    my_aiocb.aio_buf = buf; //(char *)malloc(sizeof(BUFFER_SIZE + 1));
    my_aiocb.aio_nbytes = 1024; //BUFFER_SIZE;
    my_aiocb.aio_offset = 0;

    //填充aiocb中有关回调通知的结构体sigevent
    my_aiocb.aio_sigevent.sigev_notify = SIGEV_THREAD;//使用线程回调通知
    my_aiocb.aio_sigevent.sigev_notify_function = aio_completion_handler;//设置回调函数
    my_aiocb.aio_sigevent.sigev_notify_attributes = NULL;//使用默认属性
    my_aiocb.aio_sigevent.sigev_value.sival_ptr = &my_aiocb;//在aiocb控制块中加入自己的引用

    //异步读取文件
    ret = aio_write(&my_aiocb);
    if(ret < 0)
    {
        perror("aio_write");
    }

    sleep(1);


    return 0;
}
