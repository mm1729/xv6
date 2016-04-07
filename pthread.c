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
    return 0;
  }

int pthread_join(pthread_t thread, void** retval)
{
  return 0;
}
int pthread_exit(void* retval)
{
  return 0;
}
