
//#define _RAND_WRITE
//#define _CHANGE_IO_RATE
#define _PRINT_IO_LATENCY
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#define BUFFER_SIZE 1048576
#define RAND_RANGE 20971520	// 20M

/*
*线程池里所有运行和等待的任务都是一个CThread_worker
*由于所有任务都在链表里，所以是一个链表结构
*/
typedef struct worker
{
    /*回调函数，任务运行时会调用此函数，注意也可声明成其它形式*/
    void *(*process) (void *arg);
    void *arg;/*回调函数的参数*/
    struct worker *next;

} CThread_worker;

/*线程池结构*/
typedef struct
{
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_ready;

    /*链表结构，线程池中所有等待任务*/
    CThread_worker *queue_head;

    /*是否销毁线程池*/
    int shutdown;
    pthread_t *threadid;
    /*线程池中允许的活动线程数目*/
    int max_thread_num;
    /*当前等待队列的任务数目*/
    int cur_queue_size;

} CThread_pool;

struct Request {
	unsigned long offset;
	int length;
};

int fd;
char * buf;

int pool_add_worker (void *(*process) (void *arg), void *arg);
void *thread_routine (void *arg);

//share resource
static CThread_pool *pool = NULL;
void
pool_init (int max_thread_num)
{
    pool = (CThread_pool *) malloc (sizeof (CThread_pool));

    pthread_mutex_init (&(pool->queue_lock), NULL);
    pthread_cond_init (&(pool->queue_ready), NULL);

    pool->queue_head = NULL;

    pool->max_thread_num = max_thread_num;
    pool->cur_queue_size = 0;

    pool->shutdown = 0;

    pool->threadid = (pthread_t *) malloc (max_thread_num * sizeof (pthread_t));
    int i = 0;
    for (i = 0; i < max_thread_num; i++)
    { 
        pthread_create (&(pool->threadid[i]), NULL, thread_routine, NULL);
    }
}

/*向线程池中加入任务*/
int
pool_add_worker (void *(*process) (void *arg), void *arg)
{
    /*构造一个新任务*/
    CThread_worker *newworker = (CThread_worker *) malloc (sizeof (CThread_worker));
    newworker->process = process;
    newworker->arg = arg;
    newworker->next = NULL;/*别忘置空*/

    pthread_mutex_lock (&(pool->queue_lock));
    /*将任务加入到等待队列中*/
    CThread_worker *member = pool->queue_head;
    if (member != NULL)
    {
        while (member->next != NULL)
            member = member->next;
        member->next = newworker;
    }
    else
    {
        pool->queue_head = newworker;
    }

    assert (pool->queue_head != NULL);

    pool->cur_queue_size++;
    pthread_mutex_unlock (&(pool->queue_lock));
    /*好了，等待队列中有任务了，唤醒一个等待线程；
    注意如果所有线程都在忙碌，这句没有任何作用*/
    pthread_cond_signal (&(pool->queue_ready));
    return 0;
}

/*销毁线程池，等待队列中的任务不会再被执行，但是正在运行的线程会一直
把任务运行完后再退出*/
int
pool_destroy ()
{
    if (pool->shutdown)
        return -1;/*防止两次调用*/
    pool->shutdown = 1;

    /*唤醒所有等待线程，线程池要销毁了*/
    pthread_cond_broadcast (&(pool->queue_ready));

    /*阻塞等待线程退出，否则就成僵尸了*/
    int i;
    for (i = 0; i < pool->max_thread_num; i++)
        pthread_join (pool->threadid[i], NULL);
    free (pool->threadid);

	if(pool->queue_head != NULL) {
		printf("%d\n", pool->cur_queue_size);
	}

    /*销毁等待队列*/
    CThread_worker *head = NULL;
    while (pool->queue_head != NULL)
    {
        head = pool->queue_head;
        pool->queue_head = pool->queue_head->next;
        free (head);
    }
    /*条件变量和互斥量也别忘了销毁*/
    pthread_mutex_destroy(&(pool->queue_lock));
    pthread_cond_destroy(&(pool->queue_ready));
    
    free (pool);
    /*销毁后指针置空是个好习惯*/
    pool=NULL;
    return 0;
}

int ct;

void *
thread_routine (void *arg)
{
    //printf ("starting thread 0x%x\n", pthread_self ());
    while (1)
    {
        pthread_mutex_lock (&(pool->queue_lock));
        /*如果等待队列为0并且不销毁线程池，则处于阻塞状态; 注意
        pthread_cond_wait是一个原子操作，等待前会解锁，唤醒后会加锁*/
        while (pool->cur_queue_size == 0 && !pool->shutdown)
        {
            //printf ("thread 0x%x is waiting\n", pthread_self ());
            pthread_cond_wait (&(pool->queue_ready), &(pool->queue_lock));
        }

        /*线程池要销毁了*/
        if (pool->cur_queue_size == 0 && pool->shutdown)
        {
            /*遇到break,continue,return等跳转语句，千万不要忘记先解锁*/
            pthread_mutex_unlock (&(pool->queue_lock));
            //printf ("thread 0x%x will exit\n", pthread_self ());
            pthread_exit (NULL);
        }

        //printf ("thread 0x%x is starting to work\n", pthread_self ());

        /*assert是调试的好帮手*/
        assert (pool->cur_queue_size != 0);
        assert (pool->queue_head != NULL);

		/*
		if(++ct == 10000) {
			gettimeofday(&end, NULL);
			int d = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
			printf("time=%d\n", d);
			ct = 0;
		}
		*/

        /*等待队列长度减去1，并取出链表中的头元素*/
        pool->cur_queue_size--;
        CThread_worker *worker = pool->queue_head;
        pool->queue_head = worker->next;
        pthread_mutex_unlock (&(pool->queue_lock));

        /*调用回调函数，执行任务*/
        (*(worker->process)) (worker->arg);
        free (worker);
        worker = NULL;
    }
    /*这一句应该是不可达的*/
    pthread_exit (NULL);
}

time_t startTimeStamp;

void *
myprocess (void *arg)
{
    struct Request *req = (struct Request*) arg;

#ifdef _PRINT_IO_LATENCY
    struct timeval ts, te;
	gettimeofday(&ts, NULL);
#endif

#ifdef _CHANGE_IO_RATE
	pwrite(fd, buf, 4096, req->offset);
#else
	pwrite(fd, buf, req->length, req->offset);
#endif

#ifdef _PRINT_IO_LATENCY
	gettimeofday(&te, NULL);
	time_t req_ts = 1000000 * ts.tv_sec + ts.tv_usec;
	time_t req_time = req_ts - startTimeStamp;
	time_t latency = 1000000 * te.tv_sec + te.tv_usec - req_ts;
	//printf("%u\n", latency);
	printf("%u %u\n", req_time, latency);
#endif

    return NULL;
}

int main(int argc, char **argv) {

	initialize("Cambridge/prxy_1.trace");

	fd = open("/dev/rbd0", O_RDWR|O_DIRECT);
	posix_memalign((void **)&buf, 512, BUFFER_SIZE);
	memset(buf, 'c', BUFFER_SIZE);

	int num_threads = 128;
	pool_init(num_threads);

	//double t;
	//double t0 = 1172165502415434;

#ifdef _CHANGE_IO_RATE
	int step;
	#ifdef _RAND_WRITE
		srand((int) time(0));
		step = 2048;
		int emit_interval[80] = {2815,4644,6311,8525,5148,6288,6212,8075,5443,6193,4386,9567,8966,5766,
								6910,2884,9594,2500,5444,3906,6863,8396,3094,8071,8668,6595,3592,
								4019,3951,3343,9446,6323,4955,4865,3674,7687,4410,8943,9222,3863,5540,
								7662,8856,5992,5078,3145,5657,3586,7135,6457,9094,8744,6197,2907,
								8868,6704,5215,6827,8915,2624,9957,6784,9554,6667,6267,8804,7539,9619,
								5243,3094,4554,8520,5012,7798,7260,9428,6610,9022,4399,6963};
	#else
		/*step = 16384;
		int emit_interval[80] = {975,573,1675,779,1097,827,1122,544,756,1516,778,1631,1515,1468,1088,1843,697,849,1320,733,1058,
								1221,1055,1941,815,1862,1231,1937,1548,1920,1511,728,1228,600,1907,937,1255,1215,1049,871,1933,1826,
								693,1047,1505,1047,1319,1805,1723,1380,989,1697,1979,1554,1330,1651,1926,1662,1315,1185,756,1008,1569,
								961,883,568,1526,811,733,1726,1707,1114,1982,1606,721,1091,1210,1499,1318,1843};*/

		//step = 16384;
		//int emit_interval[20] = {500, 500, 3000, 500, 3000, 500, 3000, 500, 3000, 500,
		//						3000, 500, 3000, 500, 3000, 500, 3000, 500, 3000, 500};

		//high
		//step = 32768;
		//int emit_interval[20] = {400, 400, 400, 3000, 400, 400, 3000, 400, 400, 3000,
		//						400, 400, 3000, 400, 400, 3000, 400, 400, 3000, 400};

		//mid
		//step = 28000;
		//int emit_interval[20] = {500, 500, 500, 3000, 500, 500, 3000, 500, 500, 3000,
		//						500, 500, 3000, 500, 500, 3000, 500, 500, 3000, 500};

		//low
		//step = 24000;
		//int emit_interval[20] = {500, 500, 500, 4000, 500, 500, 4000, 500, 500, 4000,
		//						500, 500, 4000, 500, 500, 4000, 500, 500, 4000, 500};
	#endif
#endif

	short devno;
	char rwType;
	double reqtime;
	struct Request req;
	int i;

	struct timeval start;
	gettimeofday(&start, NULL);
	startTimeStamp = 1000000 * start.tv_sec + start.tv_usec;

	for(i = 0; i < 700000; ++i) {
	#ifdef _RAND_WRITE
		req.offset = (unsigned long)(rand() % RAND_RANGE) << 12;
	#else
		nextrequest(&devno, &req.offset, &req.length, &rwType, &reqtime);
	#endif
		/*
		gettimeofday(&end, NULL);
		t = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
		int d = (int)((req.reqtime - t0 - t));
		if(d > 0)	usleep(d);
		*/
	#ifdef _CHANGE_IO_RATE
		usleep(emit_interval[i/step]);
	#else
		usleep(500);
	#endif
		pool_add_worker(myprocess, &req);	
	}

	sleep(10);

    pool_destroy ();
	finalize();
	free(buf);
	close(fd);

	return 0;
}
