/*
This file contains interface functions with java, such as
initializing the setting(Java_com_SMU_DevSec_CacheScan_init),
scanning(Java_com_SMU_DevSec_SideChannelJob_scan),
trial mode check(Java_com_SMU_DevSec_TrialModelStages_trial1).
*/
using namespace std;

#include <jni.h>
#include <string>
#include <cstring>
#include <dlfcn.h>
#include <libflush/libflush.h>
#include <libflush/hit.h>
#include <libflush/calibrate.h>
#include <linux/ashmem.h>
#include <asm/fcntl.h>
#include <fstream>
#include <asm/mman.h>
#include <linux/mman.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
//#include <libflush/lock.h>
#include "split.c"
#include "ReadOffset.h"
#include "logoutput.h"

#include <android/sharedmem.h>
#include <android/sharedmem_jni.h>

int finishtrial1 = 0;
jint *filter;
int t[] = {0, 0};

int firstrun = 1;
int continueRun = 0;
int threshold = 0;
static long timingCount = 0;
static long *timingCountptr = &timingCount;
int *flags;
int sum_length = 0;
size_t *addr = nullptr;

int running = 0;
int *camera_pattern;
int *audio_pattern;
int camera_audio[] = {1, 2};//indexes of camera list and audio list
std::vector<std::string> camera_list;
std::vector<std::string> audio_list;
size_t *mfiles;

long *buffer = NULL;

struct memArea {
    int *map;
    int fd;
    int size;
};
struct memArea maps[10];
int num = 0;

int compiler_position = 5;
int log_length = 0;

int logs[100000] = {0};
long times[100000] = {0};
long addresses[100000] = {0};
long timingCounts[100000] = {0};
int thresholds[100000] = {0};
int length_of_camera_audio[2] = {0, 0};
static int *shared_data_shm_fd;
static int *shared_data;
int *shared_data_ptr;

int hitCounts[10] = {0};
int pauses[100000] = {1};


pthread_mutex_t g_lock;
pthread_mutex_t shared_mem_lock;

libflush_session_t *libflush_session;


extern "C" JNIEXPORT jstring JNICALL
Java_threads_thor_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}




extern "C"
JNIEXPORT int JNICALL
Java_threads_thor_MainActivity_setSharedMap(JNIEnv *env, jobject thiz) {
    int pp = 0;
    int pp1 = 0;
    if (shared_data_ptr == NULL) {
        LOGD("shared_data_shm rainview null");
        int pp = set_shared_mem(*shared_data_shm_fd, shared_data, &shared_mem_lock);
        LOGD("shared_data_shm splash shared_data_shm_fd %d", pp);
        pp1 = pp;
        //    set_shared_mem_val(pp, &shared_mem_lock);
        shared_data_ptr = reinterpret_cast<int *>(pp);
    }
    LOGD("shared_data_shm splash shared_data_shm_fd %d", pp1);

    return pp1;
}


static void setAshMemVal(jint fd, jlong val) {

    if (buffer == NULL) {
        buffer = (long *) mmap(NULL, 128, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    }
    buffer[0] = val;
}

static jlong readAshMem(jint fd) {
    if (buffer == NULL) {
        buffer = (long *) mmap(NULL, 128, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    }
//    LOGD("read_ashmem %ld", buffer[0]);

    return buffer[0];
}


//extern "C"
//JNIEXPORT int JNICALL
//Java_threads_thor_MainActivity_createAshMem(
//        JNIEnv *env,
//        jobject /* this */) {
//    int fd = open("/" ASHMEM_NAME_DEF, O_RDWR);
//    LOGD("create ashmem %d", fd);
//    ioctl(fd, ASHMEM_SET_NAME, "memory");
//    ioctl(fd, ASHMEM_SET_SIZE, 128);
//
//    buffer = (long *) mmap(NULL, 128, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
//    return fd;
//}

extern "C"
JNIEXPORT int JNICALL
Java_threads_thor_MainActivity_createAshMem31(
        JNIEnv *env,
        jobject /* this */) {
    int fd = ASharedMemory_create("memory", 128);
    buffer = (long *) mmap(NULL, 128, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    return fd;
}

extern "C" JNIEXPORT long JNICALL
Java_threads_thor_MainActivity_readAshMem(
        JNIEnv *env,
        jobject thiz, jint fd) {

    return readAshMem(fd);

//    if (buffer == NULL) {
//        buffer = (long *) mmap(NULL, 128, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
//    }
//    return buffer[0];
}


extern "C" JNIEXPORT long JNICALL
Java_threads_thor_aspects_AspectLoggingJava_readAshMem1(
        JNIEnv *env,
        jobject thiz, jint fd) {

    return readAshMem(fd);

//    if (buffer == NULL) {
//        buffer = (long *) mmap(NULL, 128, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
//    }
//    return buffer[0];
}

extern "C" JNIEXPORT void JNICALL
Java_threads_thor_MainActivity_setAshMemVal(
        JNIEnv *env,
        jobject thiz, jint fd, jlong val) {

    setAshMemVal(fd, val);
//    if (buffer == NULL) {
//        buffer = (long *) mmap(NULL, 128, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
//    }
//    buffer[0] = val;
}


/*
This file contains interface functions with java, such as
initializing the setting(Java_com_SMU_DevSec_CacheScan_init),
scanning(Java_com_SMU_DevSec_SideChannelJob_scan),
trial mode check(Java_com_SMU_DevSec_TrialModelStages_trial1).
*/

int address_check(std::string function) {
    //std::string a[] = {"AudioManager.javaupdateAudioPortCache","AudioVolumeGroupChangeHandler.java<init>","AudioMixPort.javabuildConfig","AudioManager.javaupdatePortConfig","AudioManager.javabroadcastDeviceListChange_sync","AudioDevicePort.javabuildConfig","AudioAttributes.java<init>","AudioManager.javainfoListFromPortList","AudioRecord.java<init>","AudioAttributes.java<init>","AudioPortEventHandler.javahandleMessage","CallAudioState.java<init>","AudioManager.javagetDevices","AudioHandle.javaequals","AudioManager.javacalcListDeltas","CameraMetadataNative.java<init>","CameraMetadataNative.javaregisterAllMarshalers","CameraCharacteristics.javaget","CameraMetadataNative.javanativeClose","CameraManager.javagetCameraIdList","CameraMetadataNative.javanativeGetTypeFromTag","CameraManager.javaconnectCameraServiceLocked","CameraManager.javaonTorchStatusChangedLocked","CameraManager.javacompare","ICameraService.javaisHiddenPhysicalCamera","CameraManager.javaonStatusChangedLocked","CameraManager.javaonTorchStatusChanged","CameraCharacteristics.java<init>","ICameraServiceProxy.javaonTransact","CameraManager.javacompare","ICameraServiceProxy.java<init>","ICameraService.javagetCameraCharacteristics","CameraMetadataNative.javanativeReadValues","CameraMetadataNative.javanativeWriteToParcel"};
    //std::string a[] = {"AudioHandle.javaequals","AudioManager.javaupdateAudioPortCache","AudioManager.javabroadcastDeviceListChange_sync","AudioManager.javacalcListDeltas","AudioManager.javaupdatePortConfig","AudioPortEventHandler.javahandleMessage","AudioRecord.java<init>","CameraManager.javacompare","CameraManager.javagetCameraIdList","CameraMetadataNative.javanativeReadValues","CameraMetadataNative.javanativeWriteToParcel","CameraMetadataNative.javaregisterAllMarshalers","ICameraService.javagetCameraCharacteristics","ICameraService.javaisHiddenPhysicalCamera","ICameraServiceProxy.java<init>","CameraManager.javaconnectCameraServiceLocked","CameraManager.javaonTorchStatusChangedLocked","CameraManager.javagetCameraIdList","CameraManager.javaonTorchStatusChanged"};
    std::string a[] = {"AudioVolumeGroupChangeHandler.java<init>",
                       "AudioManager.javainfoListFromPortList", "AudioDevicePort.javabuildConfig",
                       "AudioHandle.javaequals", "AudioManager.javabroadcastDeviceListChange_sync",
                       "AudioManager.javacalcListDeltas", "AudioPortEventHandler.javahandleMessage",
                       "AudioManager.javaupdateAudioPortCache", "AudioManager.javaupdatePortConfig",
                       "AudioRecord.java<init>", \
    "CameraMetadataNative.javanativeClose", "CameraManager.javagetCameraIdList",
                       "CameraMetadataNative.javanativeReadValues",
                       "CameraMetadataNative.javanativeWriteToParcel",
                       "CameraMetadataNative.javaregisterAllMarshalers",
                       "ICameraService.javaisHiddenPhysicalCamera",
                       "ICameraServiceProxy.java<init>",
                       "CameraManager.javaconnectCameraServiceLocked",
                       "CameraManager.javaonTorchStatusChanged"};
    size_t cnt = sizeof(a) / sizeof(std::string);
    for (int i = 0; i < cnt; i++) {
        if (function == a[i])
            return 1;
    }
    return 0;
}


extern "C" JNIEXPORT void JNICALL
Java_threads_thor_SideChannelJob_scan(
        JNIEnv *env,
        jobject thiz, jintArray ptfilter) {
    continueRun = 1;
    if (firstrun !=
        1) {//since if we run this function repeatedly, the app tend to crash, so we only pause the thread.
        LOGD("Restart scanning.");
        return;
    }
    firstrun = 0;
    //ptfilter is a filter used to ignore some functions
    int *arrp = env->GetIntArrayElements(ptfilter, 0);
    for (int i = 0; i < length_of_camera_audio[0]; i++)//camera
    {
        if (arrp[i] == 1) {
            *((size_t *) addr[camera_audio[0]] + i) = 0;
        }
    }
    for (int i = length_of_camera_audio[0];
         i < length_of_camera_audio[0] + length_of_camera_audio[1]; i++)//audio
    {
        if (arrp[i] == 1) {
            *((size_t *) addr[camera_audio[1]] + i - length_of_camera_audio[0]) = 0;
        }
    }
    for (int i = 1; i < 3; i++) {
        int c = i - 1;
        int t0 = 0;
        int t1 = 0;
        for (int j = 0; j < length_of_camera_audio[c]; j++) {
            size_t target = *((size_t *) addr[i] + j);
            if (target == 0) {//if the target is 0, skip it.
                t0++;
                continue;
            }
            t1++;
        }
        if (i == 1) {
//            LOGD("In Camera List, %d are null functions, %d are available functions.\n", t0, t1);
            t[0] = t1;
        } else {
//            LOGD("In Audio List, %d are null functions, %d are available functions.\n", t0, t1);
            t[1] = t1;
        }
    }
    hit(&g_lock, compiler_position, &continueRun,
        threshold, flags, times, thresholds, logs, &log_length, sum_length,
        addr, camera_pattern, audio_pattern,
        length_of_camera_audio); //start scannning. in libflush/hit.c
    //running = 0;
    LOGD("Finished scanning %d", running);
    return;
}


extern "C" JNIEXPORT long JNICALL
Java_threads_thor_SideChannelJob_scan7(
        JNIEnv *env,
        jobject thiz, jlongArray arr, jint length, jint pauseVal, jint hitVal,
        jboolean resetHitCounter) {

    jlong *arrp;
    arrp = env->GetLongArrayElements(arr, 0);
    size_t *addr;
    // do some exception checking
    if (arrp == NULL) {
        LOGD("scan4  null pointer arrp");

        return -1; /* exception occurred */
    }

    jint i = 0;
    for (i = 0; i < length; i++) {
        if (arrp + i == NULL) {
            LOGD("scan4 null %d", i);
            break;
        }

    }

    hit7(arrp, length, threshold, &timingCount, times, logs, timingCounts, addresses, &log_length,
         &g_lock, buffer, libflush_session, hitCounts, pauses, pauseVal, hitVal, resetHitCounter);
//    LOGD("Finished scanning %d", running);
    (env)->ReleaseLongArrayElements(arr, arrp, 0);

    return timingCount;
}




extern "C" JNIEXPORT long JNICALL
Java_threads_thor_SideChannelJob_readAshMem(
        JNIEnv *env,
        jobject thiz, jint fd) {

    return readAshMem(fd);
//    if (buffer == NULL) {
//        buffer = (long *) mmap(NULL, 128, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
//    }
//    return buffer[0];
}

extern "C" JNIEXPORT void JNICALL
Java_threads_thor_SideChannelJob_setAshMemVal(
        JNIEnv *env,
        jobject thiz, jint fd, jlong val) {

    setAshMemVal(fd, val);
//    if (buffer == NULL) {
//        buffer = (long *) mmap(NULL, 128, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
//    }
//    buffer[0] = val;
}




extern "C" JNIEXPORT void JNICALL
Java_threads_thor_SideChannelJob_scanOdex(
        JNIEnv *env,
        jobject thiz, jlongArray arr, jint length) {

    jlong *c_array;
    jint i = 0;

    // get a pointer to the array
    c_array = (env)->GetLongArrayElements(arr, 0);

    // do some exception checking
    if (c_array == NULL) {
        return; /* exception occurred */
    }

    // do stuff to the array
    for (i = 0; i < length; i++) {
        scanOdexMemory(reinterpret_cast<void *>(c_array[i]), 0, 0);
    }

    // release the memory so java can have it again
    (env)->ReleaseLongArrayElements(arr, c_array, 0);

    LOGD("Finished scanning %d", running);
    return;
}


extern "C" JNIEXPORT void JNICALL
Java_threads_thor_SideChannelJob_adjustThreshold(JNIEnv *env, jobject thiz,
                                                 jlongArray arr, jint length) {
    jlong *arrp;
    arrp = env->GetLongArrayElements(arr, 0);
    size_t *addr;
    // do some exception checking
    if (arrp == NULL) {
        LOGD("adjust_threshold null pointer arrp");

        return; /* exception occurred */
    }
//    set_shared_mem_val(shared_data_ptr, 7, &shared_mem_lock);
    jint i = 0;
    for (i = 0; i < length; i++) {
        if (arrp + i == NULL) {
            LOGD("adjust_threshold null %d", i);
            break;
        }
//        size_t target = *((size_t *) addr[i]);
//        LOGD("scan4 %lu", arrp[i]);
//        *(addr + i) = arrp[i];



    }
    LOGD("adjust_threshold1 threshold before %d ", threshold);

    threshold = adjust_threshold2(arrp, length, threshold, libflush_session);
    LOGD("adjust_threshold1 final threshold %d ", threshold);

    //    LOGD("Finished scanning %d", running);
    (env)->ReleaseLongArrayElements(arr, arrp, 0);

    return;
}

template<typename T>
void swap(T *a, T *b) {
    T temp;
    temp = *a;
    *a = *b;
    *b = temp;
}


extern "C" JNIEXPORT void JNICALL
Java_threads_thor_TrialModelStages_trial1(
        JNIEnv *env, jobject thiz) {
    LOGD("Start trial 1.\n");
    int length_alive_function = length_of_camera_audio[0] + length_of_camera_audio[1];
//    stage1_(filter, threshold, length_of_camera_audio, addr, camera_audio, &finishtrial1,sum_length); //eliminate functions that keeps poping.
    LOGD("Finish TrialMode 1");
    return;
}

extern "C" JNIEXPORT void JNICALL
Java_threads_thor_SideChannelJob_trial2(
        JNIEnv *env, jobject thiz) {
    LOGD("Start trial 2.\n");
    continueRun = 1;
    hit(&g_lock, compiler_position, &continueRun,
        threshold, flags, times, thresholds, logs, &log_length, sum_length,
        addr, camera_pattern, audio_pattern, length_of_camera_audio);
    LOGD("Finish TrialMode 2");
}


/**
 * Function to initialize the setting
 */
extern "C" JNIEXPORT jstring JNICALL
Java_threads_thor_CacheScan_init(
        JNIEnv *env,
        jobject thiz,
        jobjectArray dexlist, jobjectArray filenames, jobjectArray func_lists) {
    jsize size = env->GetArrayLength(dexlist);//get number of dex files
    char **func_list; //functions' offsets of every library;
    //get address list
//    mfiles = static_cast<size_t *>(malloc(sizeof(size_t *) * size));
//    for(int i=0;i<size;i++)
//    {
//        jstring obj = (jstring)env->GetObjectArrayElement(dexlist,i);
//        std::string dex = env->GetStringUTFChars(obj,NULL);
//        obj = (jstring)env->GetObjectArrayElement(filenames,i);
//        std::string filename = env->GetStringUTFChars(obj,NULL);
//        obj = (jstring)env->GetObjectArrayElement(func_lists,i);
//        int length=0;      func_list  = split(',',(char*)env->GetStringUTFChars(obj,NULL), &length);//split a string into function list
//        LOGD("Filename %s, Length %d.", filename.c_str(), length);
//        //expand addr[];
//        sum_length = sum_length + length;
//        addr = static_cast<size_t *>(realloc(addr,sum_length*sizeof(size_t)));
//        mfiles[i] = ReadOffset(env, dex, addr, func_list, length, filename, camera_list, audio_list);//read the offsets of functions. In ReadOffset.h
//    }
//    LOGD("Functions Length %d",sum_length);
//    LOGD("Camera List: %d, Audio List: %d",length_of_camera_audio[0],length_of_camera_audio[1]);
//    Should be enabled
    threshold = 100;
    libflush_init(&libflush_session, NULL);

    threshold = get_threshold_wsessiom(libflush_session);
    timingCountptr = &timingCount;

//    threshold = adjust_threshold(threshold, length_of_camera_audio, addr, camera_audio, &finishtrial1);//adjust threshold
//    threshold = adjust_threshold1(threshold, addr, 1);//adjust threshold

    return env->NewStringUTF("");
}

extern "C"
JNIEXPORT void JNICALL
Java_threads_thor_SideChannelJob_pause(JNIEnv *env, jobject thiz) {
    // to stop scanning;
    continueRun = 0;
}

extern "C"
JNIEXPORT int JNICALL
Java_threads_thor_activities_RainViewerActivity_nativeChildBMethodB(JNIEnv *env,
                                                                    jobject thiz) {
    int ans = rand() % 50;
    LOGD("hjhsjgfjhsf jkdhjksdh %f", ans + sqrt(ans));
    return ans;
}


extern "C" JNIEXPORT void JNICALL
Java_threads_thor_SideChannelJob_setSharedMapChild(JNIEnv *env, jobject thiz,
                                                   jint shared_mem_ptr,
                                                   jcharArray fileDes) {

    jchar *arrp;
    arrp = env->GetCharArrayElements(fileDes, 0);
    size_t *addr;
    set_shared_mem_child(arrp, &shared_mem_lock);


//    shared_data_ptr = reinterpret_cast<int *>(shared_mem_ptr);
//    set_shared_mem_val(shared_data_ptr, 8, &shared_mem_lock);
//    int ans = get_shared_mem_val(shared_data_ptr, &shared_mem_lock);
//    LOGD("shared_data_shm side channel ans %d", ans);

}


extern "C" JNIEXPORT void JNICALL
Java_threads_thor_MainActivity_setSharedMapChildTest(JNIEnv *env,
                                                     jobject thiz,
                                                     jint shared_mem_ptr,
                                                     jcharArray fileDes) {

    LOGD("shared_data_shm inside setSharedMapChildTest");

    jchar *arrp;
    arrp = env->GetCharArrayElements(fileDes, 0);
    size_t *addr;
    set_shared_mem_child(arrp, &shared_mem_lock);


//    shared_data_ptr = reinterpret_cast<int *>(shared_mem_ptr);
//    set_shared_mem_val(shared_data_ptr, 8, &shared_mem_lock);
//    int ans = get_shared_mem_val(shared_data_ptr, &shared_mem_lock);
//    LOGD("shared_data_shm side channel ans %d", ans);

}

int *map;
int size;

static void setmap(JNIEnv *env, jclass cl, jint fd, jint sz) {
    size = sz;
    map = (int *) mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

static jint setNum(JNIEnv *env, jclass cl, jint pos, jint num) {
    if (pos < (size / sizeof(int))) {
        map[pos] = num;
        return 0;
    }
    return -1;
}

static jint getNum(JNIEnv *env, jclass cl, jint pos) {
    if (pos < (size / sizeof(int))) {
        return map[pos];
    }
    return -1;
}


static JNINativeMethod method_table[] = {
        {"setVal", "(II)I", (void *) setNum},
        {"getVal", "(I)I",  (void *) getNum},
        {"setMap", "(II)V", (void *) setmap}

};


