/*
 * Thread Pool
 */

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define DEFAULT 2
#define THREADS 4
#define WORKERS 10

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

/* Workers */
work_T *work_create(thread_function_T f, void *arg) {
  if (f == NULL) return NULL;

  work_T *work = malloc(sizeof(work_T));

  work->function = f;
  work->arg = arg;
  work->next = NULL;

  return work;
} 

void work_destroy(work_T *work) {
  if (work == NULL) return;
  free(work);
}

work_T *work_get(thread_pool_T *t) {
  if (t == NULL) return NULL;

  work_T *work = t->first;
  if (work->next == NULL) {
    t->first = NULL;
    t->last = NULL;
  }
  else {
    t->first = work->next;
  }

  return work;
}

void *worker(void *arg) {
  thread_pool_T *t = arg;
  work_T *work;

  while (1) {
    pthread_mutex_lock(&(t->work_mutex));

    while (t->first == NULL && !t->stop) {
      pthread_cond_wait(&(t->work_cond), &(t->work_mutex));
    }

    if (t->stop) break;

    work = work_get(t);
    t->working_count++;

    pthread_mutex_unlock(&(t->work_mutex));

    if (work != NULL) {
      work->function(work->arg); // run
      work_destroy(work);
    }

    pthread_mutex_lock(&(t->work_mutex));
    t->working_count--;

    if (!t->stop && t->working_count == 0 && t->first == NULL) {
      pthread_cond_signal(&(t->working_cond));
    }

    pthread_mutex_unlock(&(t->work_mutex));
  }

  t->thread_count--;
  pthread_cond_signal(&(t->working_cond));
  pthread_mutex_unlock(&(t->work_mutex));

  return NULL;
}
thread_pool_T *create_threadpool(size_t n) {
  if (n <= 0) n = DEFAULT;

  pthread_t thread; 

  thread_pool_T *t = malloc(sizeof(thread_pool_T));
  t->thread_count = n;

  pthread_mutex_init(&(t->work_mutex), NULL);
  pthread_cond_init(&(t->work_cond), NULL);
  pthread_cond_init(&(t->working_cond), NULL);

  t->first = NULL;
  t->last = NULL;

  for (int i = 0; i < n; i++) {
    pthread_create(&thread, NULL, worker, t);
    pthread_detach(thread);
  }

  return t;
}

void threadpool_wait(thread_pool_T *t) {
  if (t == NULL) return;
  
  pthread_mutex_lock(&(t->work_mutex));

  while (1) { 
    if ((!t->stop && t->working_count != 0) || (t->stop && t->thread_count != 0)) {
      pthread_cond_wait(&(t->working_cond), &(t->work_mutex));
    }
    else break;
  }

  pthread_mutex_unlock(&(t->work_mutex));
}

void destroy(thread_pool_T *t) {
  work_T *w1, *w2;

  if (t == NULL) return;

  pthread_mutex_lock(&(t->work_mutex));
  w1 = t->first;

  while (w1 != NULL) {
    w2 = w1->next;
    work_destroy(w1);
    w1 = w2;
  }

  t->stop = true;
  pthread_cond_broadcast(&(t->work_cond));
  pthread_mutex_unlock(&(t->work_mutex));

  threadpool_wait(t);

  pthread_mutex_destroy(&(t->work_mutex));
  pthread_cond_destroy(&(t->work_cond));
  pthread_cond_destroy(&(t->working_cond));

  free(t);
}

bool add(thread_pool_T *t, thread_function_T f, void *arg) {
  
  if (t == NULL) return false;

  work_T *work = work_create(f,arg);

  if (work == NULL) return false;

  pthread_mutex_lock(&(t->work_mutex));

  if (t->first == NULL) {
    t->first = work;
    t->last = t->first;
  } else {
    t->last->next = work;
    t->last = work;
  }

  pthread_cond_broadcast(&(t->work_cond));
  pthread_mutex_unlock(&(t->work_mutex));

  return true;
}


void hello(void *arg) {
  int *val = arg;

  *val += 1;
  printf("Hello %d\n", val);

  if (*val % 2) usleep(100000);
}

/* Driver */
int main() {
  thread_pool_T *t = create_threadpool(THREADS);
  int *vals = calloc(WORKERS, sizeof(int));
  for (int i = 0; i < WORKERS; i++) {
    vals[i] = i;
    add(t, hello, vals+i);
  }

  threadpool_wait(t);

  for (int i = 0; i < WORKERS; i++) {
    printf("%d\n", vals[i]);
  }

  free(vals);
  destroy(t);


  return 0;
}


