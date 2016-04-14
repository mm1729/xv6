#include "spinlock.h"

typedef struct{

  struct spinlock lock;
  int valid;
  int status;
}mutex_t;

typedef struct{
  mutex_t mutex_arr[32];
  struct spinlock lock;

}mutex_table;
