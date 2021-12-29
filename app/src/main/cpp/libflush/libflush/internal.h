/* See LICENSE file for license and copyright information */

#ifndef INTERNAL_H
#define INTERNAL_H

#include "libflush.h"
#include <pthread.h>

typedef struct thread_data_s {
  libflush_session_t* session;
  ssize_t cpu;
} thread_data_t;


struct libflush_session_s {
  void* data;
  bool performance_register_div64;
    struct {
        pthread_t thread;
        volatile uint64_t value; //number for counter
        thread_data_t data;
    } thread_counter;

#if HAVE_PAGEMAP_ACCESS == 1
  struct {
    int pagemap;
  } memory;
#endif

#if TIME_SOURCE == TIME_SOURCE_THREAD_COUNTER
//  struct {
//    pthread_t thread;
//    volatile uint64_t value; //number for counter
//    thread_data_t data;
//  } thread_counter;
#endif

#if TIME_SOURCE == TIME_SOURCE_PERF
  struct {
    int fd;
  } perf;
#endif
};

#endif  /*INTERNAL_H*/
