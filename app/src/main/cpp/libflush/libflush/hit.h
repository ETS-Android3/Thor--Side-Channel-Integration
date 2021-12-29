//
// Created by finder on 20-10-16.
//
#ifndef DEVSEC_HIT_H
#define DEVSEC_HIT_H

#ifdef __cplusplus
extern "C" {
#endif


#include "libflush.h"
void stage1_(int* arr,size_t threshold, int* length_of_camera_audio, size_t* addr, int* camera_audio, int* finishtrial1,int sum_length);
void flush_address(size_t* address,int length);
int hit(pthread_mutex_t *g_lock, int compiler_position, int *continueRun, int threshold, int *flags,
        long *times, int *thresholds, int *logs, int *log_length, int sum_length,
        size_t *addr, int *camera_pattern, int *audio_pattern, int *length_of_camera_audio);
int hit2(char **param, int length, int threshold);
int hit5(void *param, int length, int threshold);
int hit6(jlong *param, int length, int threshold, int count);
int hit7(jlong *param, int length, int threshold, long *timingCount, long *times, int *logs,
        long *timingCounts, long *addresses, int *log_length, pthread_mutex_t *g_lock, long *buffer, libflush_session_t* libflush_session,
        int *hitCounts, int *pauses, int pauseVal, int hitVal, bool resetHitCounter);
int scanOdexMemory(void *param, int length, int threshold);
void stage1(int *arr, size_t threshold, int *length_of_camera_audio, size_t *addr, int *pInt,
            int *pInt1);
int get_threshold();
int get_threshold_wsessiom(libflush_session_t* libflush_session);
int set_shared_mem(int shared_data_shm_fd, int *shared_data, pthread_mutex_t *shared_mem_lock);
int set_shared_memfd(int shared_data_shm_fd, int *shared_data, pthread_mutex_t *shared_mem_lock);
int set_shared_mem_child(jchar *fileDes, pthread_mutex_t *shared_mem_lock );
int set_shared_mem_val(int *shared_data, int val, pthread_mutex_t *shared_mem_lock);
int get_shared_mem_val(int *shared_data, pthread_mutex_t *shared_mem_lock);
int clean_shared_mem(int shared_data_shm_fd,pthread_mutex_t *shared_mem_lock);

long get_timingCount(pthread_mutex_t *g_lock, long *timingCount);
int adjust_threshold(int threshold, int* length_of_camera_audio, size_t* addr, int* camera_audio, int* finishtrial1);
int adjust_threshold1(int threshold, size_t *addr, int length);
int adjust_threshold2(jlong *param, int length, int threshold, libflush_session_t* libflush_session);


#ifdef __cplusplus
}
#endif
#endif //DEVSEC_HIT_H
