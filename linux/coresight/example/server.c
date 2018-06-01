
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>


#define SHM_SIZE 8192  /* make it a 2 pages  shared memory segment */

unsigned int volatile  counter = 1;
char command[50];



void  funny_function1()
{

      counter = counter + 1;
      while (counter != 100)
            counter++;
      counter = 1;
}


void  funny_function2()
{

      counter = counter + 1;
      while(counter != 1000000)
         counter++ ;
      counter = 1;

}


int main(int argc, char *argv[])
{
    key_t key;
    int shmid;
    char volatile  *data;
    int mode;

    key = 4545;



    mlockall(MCL_CURRENT | MCL_FUTURE);

    /*  create the segment: */
    if ((shmid = shmget(key, SHM_SIZE, 0644 | IPC_CREAT)) == -1) {
        if ((shmid = shmget(key, SHM_SIZE, 0644 )) == -1) {
           perror("shmget");
           exit(1);
        }
    }

    /* attach to the segment to get a pointer to it: */
    data = shmat(shmid, (void *)0, 0);
    if (data == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }

 //   *((unsigned int *)data) = 0;
    while (1) {
       while (*((unsigned int *)data) == 0);

       if (*((unsigned int *)data) == 1){
          funny_function1();
          *((unsigned int *)data) = 0;
       }
       *((unsigned int *)(data+4096)) = 1;
       funny_function2();
       break;
    }
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}
