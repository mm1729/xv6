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
    thread->tid = tid;
    return tid;
  }

int pthread_join(pthread_t thread, void** retval)
{
  return join(thread->tid, thread->stack, retval);
}
int pthread_exit(void* retval)
{
  return texit(retval);
}
