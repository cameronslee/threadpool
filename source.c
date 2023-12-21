/*
 * Thread Pool
 */

#include <stdio.h>
#include <stddef.H>
#include <stdbool.h>

typedef void (*thread_function_T)(void *arg); // function pointer for task

typedef struct {
  thread_function_T function; // function call
  void *arg; // args
  struct work_T *next; 
} work_T;

typedef struct {
  work_T *first;
  work_T *last;
  pthread_mutex_t work_mutex;
  pthread_cond_t work_cond;
  pthread_cond_t working_cond;
  size_t working_count;
  size_t thread_count;
  bool stop;
} thread_pool_T;

thread_pool_T *create(size_t n) {

}

void destroy(thread_pool_T *t) {

}

bool add(thread_pool_T *t, thread_function_T function, void *arg) {

}

void wait(thread_pool_T *t) {

}

