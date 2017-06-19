#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <string.h>

#define BUFFER_SIZE 8192

int main(int argc, char **argv) {

	int fd = open("/dev/rbd0", O_RDWR|O_DIRECT);
	//char * buf = malloc(BUFFER_SIZE);
	unsigned char *buf;
    int ret = posix_memalign((void **)&buf, 512, BUFFER_SIZE);
	memset(buf, 'c', BUFFER_SIZE);
	
	int i;
	for(i = 0; i < 100000; i++) {
		write(fd, buf, BUFFER_SIZE);
	}

	free(buf);

	return 0;
}

