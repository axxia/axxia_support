
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sched.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>


#define SHM_SIZE 8192  /* make it a 1K shared memory segment */

unsigned int volatile  counter = 1;



int main(int argc, char *argv[])
{
    key_t key;
    int shmid;
    char volatile  *data;
    int mode;
    int cpu;
    int j;

    key = 4545;

    mlockall(MCL_CURRENT | MCL_FUTURE);

    /*  create the segment: */
    if ((shmid = shmget(key, SHM_SIZE, 0644 )) == -1) {
        perror("shmget");
        exit(1);
    }

    /* attach to the segment to get a pointer to it: */
    data = shmat(shmid, (void *)0, 0);
    if (data == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }
    
                                                
    *((unsigned int *)(data+4096)) = 0;
    *((unsigned int *)data) = 1;

    while (*((unsigned int *)(data+4096)) == 0);

    *((unsigned int *)(data + 4096)) = 0;

    system("killall -USR2  perf");  
    system("killall   perf");


    return 0;

}
