#ifndef XV6_PTHREAD
#define XV6_PTHREAD
// Define all functions and types for pthreads here.
// This can be included in both kernel and user code.

#define STACK_SIZE 4096


/*
  struct pthread_t
*/
typedef struct pthread_t_ {
  int pid;
  void* stack;
} pthread_t;

/*
  pthread attribute struct
*/
typedef struct pthread_attr_t_ {

} pthread_attr_t;
/*
  pthread threading functions
*/
int pthread_create(pthread_t* thread, const pthread_attr_t* attr,
  void*(*start_routine) (void*), void* arg);
int pthread_join(pthread_t thread, void** retval);
void pthread_exit(void* retval);

/*
  pthread mutex struct
*/
typedef struct pthread_mutex_t_ {
  int mutid;


}pthread_mutex_t;

/*

*/
typedef struct pthread_mutexattr_t_ {

} pthread_mutexattr_t;
/*
  pthread mutex functions
*/
int pthread_mutex_destroy(pthread_mutex_t* mutex);
int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);

#endif
