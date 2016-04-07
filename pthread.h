#ifndef XV6_PTHREAD
#define XV6_PTHREAD
// Define all functions and types for pthreads here.
// This can be included in both kernel and user code.

#define STACK_SIZE 10000

/*
  struct pthread_t
*/
typedef struct pthread_t_ {
  int tid;
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
int pthread_exit(void* retval);



#endif
