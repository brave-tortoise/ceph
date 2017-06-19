#include<stdio.h>
#include<stdlib.h>

int main(int argc, char **argv) {

	int num_threads = 128;
	pool_init(num_threads);

	initialize("Cambridge/prxy_0.trace");

	short devno;
	unsigned long offset;
	int length;
	char rwType;
	double reqtime;

	int i;
	for(i = 0; i < 10; i++) {
		nextrequest(&devno, &offset, &length, &rwType, &reqtime);
		printf("%d, %lu, %d, %c, %lf\n", devno, offset, length, rwType, reqtime);
	}


	finalize();

	return 0;
}
