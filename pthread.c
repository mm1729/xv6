#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "pthread.h"

// Implement your pthreads library here.
int pthread_create(pthread_t* thread, const pthread_attr_t* attr,
  void*(*start_routine) (void*), void* arg)
  {
    char* stack = (char *) malloc(STACK_SIZE);
    if(stack == 0) // could not allocate stack
      return -1;
    thread->stack = stack;
    int tid = clone(start_routine, arg, stack);
    //wemalloc(tid);
    thread->pid = tid;
    return tid;
  }

int pthread_join(pthread_t thread, void** retval)
{
  char* stackPtr = 0;
  int status = join(thread.pid, (void**)&stackPtr, retval);
  if(status == -1)  // error happened
    *retval = 0; // store null address in retval
  else
    if(stackPtr) free(stackPtr);

  return status;
}
void pthread_exit(void* retval)
{
  //printf(1, "Exiting %d\n", *(int *) retval);
  texit(retval);
}

int
pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr){
    int mutid = mutex_init();
    //printf(1,"%d",mutid);
    if((mutex->mutid = mutid)>=0){
      return 0;
    }
    else{
      return -1;
    }
}

int
pthread_mutex_destroy(pthread_mutex_t* mutex){
  return mutex_destroy(mutex->mutid);
}

int
pthread_mutex_lock(pthread_mutex_t* mutex){
    return mutex_lock(mutex->mutid);
}

int
pthread_mutex_unlock(pthread_mutex_t* mutex){
    return mutex_unlock(mutex->mutid);
}
