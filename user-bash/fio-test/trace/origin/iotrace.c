#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

FILE *tracefile;

int initialize(const char *filename)
{
	if ((tracefile = fopen(filename,"rb")) == NULL) {
	    fprintf(stderr, "Tracefile %s cannot be opened for read access\n", filename);
	    exit(1);
	 }
		
  return 1;
}

int nextrequest(short *devno, unsigned long *startblk, int *bytecount,char *rwType,double*reqtime)
{
	
	char line[201];
	unsigned int u_devno;
	
	if (fgets(line, 200, tracefile) == NULL) {
		if(feof(tracefile)){
			clearerr(tracefile);
			fseek(tracefile, 0L, SEEK_SET);				
		}
		
		if (fgets(line, 200, tracefile) == NULL) {
			fprintf(stderr, "Tracefile is NULL\n");
	    exit(1);
	  }		
		
	}
	
	//if (sscanf(line, "%u %lu %d %c %lf\n", &u_devno,startbyte,bytecount,rwType, reqtime) != 5) {
	if (sscanf(line, "%u %lu %d %c %lf\n", &u_devno,startblk,bytecount,rwType, reqtime) != 5) {
	    fprintf(stderr, "Wrong number of arguments for I/O trace event type\n");
	    exit(0);
	}
	*devno = (short)u_devno;
		
	if (*bytecount <0){
	    printf("mmmmmmmm\n");
	    exit(1);
	}
	
  if (*rwType=='r'){
  	*rwType='R';
  }else if(*rwType=='w'){
  	*rwType='W';
  }  	

  //change the time scale from seconds to milliseconds. Disksim uses ms as the time unit.
  *reqtime *= 1000000.0;
  	 
	return 1;
}

 
int finalize(	)
{	
	fclose(tracefile);
	return 1;
}   




