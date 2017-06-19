#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(int argc, char *argv[])
{
    int i, nu;
    srand((int) time(0));
    //for(i=0;i< 10;i++)
    //{
      //  nu = 0 + (int)( 26.0 *rand()/(RAND_MAX + 1.0));
        //printf(" %d ", nu);
    //}
    printf("%d\n", RAND_MAX);
    return 0;
}
