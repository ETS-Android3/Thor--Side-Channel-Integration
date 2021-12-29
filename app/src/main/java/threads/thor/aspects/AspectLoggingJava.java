package threads.thor.aspects;

import static threads.thor.JobMainAppInsertRunnable.insert_locker;
import static threads.thor.MainActivity.fd;
import static threads.thor.MainActivity.methodIdMap;
import static threads.thor.MainActivity.methodStats;
import static threads.thor.MainActivity.readAshMem;

import android.util.Log;

import org.aspectj.lang.ProceedingJoinPoint;
import org.aspectj.lang.annotation.Around;
import org.aspectj.lang.annotation.Aspect;
import org.aspectj.lang.annotation.Pointcut;
import org.aspectj.lang.reflect.MethodSignature;

import threads.thor.MethodStat;


@Aspect
public class AspectLoggingJava {
    static {
        System.loadLibrary("native-lib");
    }


    final String POINTCUT_METHOD_MAIN_ACTIVITY =
            "execution(* threads.thor.MainActivity.*(..))";

    @Pointcut(POINTCUT_METHOD_MAIN_ACTIVITY)
    public void executeCfgMainActivity() {
    }

    final String POINTCUT_METHOD__SETTINGS_DIALOG_FRAGMENT =
            "execution(* threads.thor.fragments.SettingsDialogFragment.*(..))";

    @Pointcut(POINTCUT_METHOD__SETTINGS_DIALOG_FRAGMENT)
    public void executeSettingsDialogFragment() {
    }

//
//    final String POINTCUT_METHOD_GAME_CONTROLLER =
//            "execution(* threads.thor.controller.GameController.handleDrag(..))";
//
//    @Pointcut(POINTCUT_METHOD_GAME_CONTROLLER)
//    public void executeCfgGameController() {
//    }


    //    @Pointcut("!within(org.woheller69.weather.activities.SplashActivity.*.*(..))")
//    public void notAspectSplashActivity() { }
//
//    @Pointcut("!within(org.woheller69.weather.activities.*.onCreate(..))")
//    public void notAspect() { }
//
//    @Around("executeManageLocationsActivity() || executeSplashRunView() || executeSplashGetRecordCount() " +
//            "|| executeRainViewer() || executeChildA() || executeChildB() || executeChildC()")
    @Around(
            "executeCfgMainActivity() "
                    + "|| executeSettingsDialogFragment() "
//                    + "|| executeCfgGameController() "
    )
//            "|| executeCfgChildC() " +
//            "|| executeCfgMethod1() " +
//            "|| executeSizeTestMethodD2() " +
//            "|| executeSizeTestMethodD1()")
//    @Around("executeCfgChildA() || executeCfgChildB() || executeCfgChildC()")
    public Object weaveJoinPoint(ProceedingJoinPoint joinPoint) throws Throwable {
        try {
            MethodSignature methodSignature = (MethodSignature) joinPoint.getSignature();
            methodIdMap.putIfAbsent(methodSignature.toString(), methodIdMap.size());

            long startFd = fd > 0 ? readAshMem(fd) : -1;
//            long startT = System.currentTimeMillis();

            Object result = joinPoint.proceed();
//            long endT = System.currentTimeMillis();

            long endFd = fd > 0 ? readAshMem(fd) : -1;

            MethodStat methodStat = new MethodStat(methodIdMap.get(methodSignature.toString()), startFd, endFd);
//            Log.d("#Aspect ", methodStat.getId()+" "+startFd+" "+endFd);
            insert_locker.lock();
            if (methodStats.isEmpty()) {
                methodStats.add(methodStat);

            } else if (!methodStats.get(methodStats.size() - 1).equals(methodStat)) {
                methodStats.add(methodStat);
            }
            insert_locker.unlock();

            Log.d("LoggingVM ", methodSignature.toString());
//            Log.v("LoggingVM ",
//                    methodSignature.toString()+" "+methodSignature.toLongString()+ methodSignature.toShortString());
            return result;
        } catch (Exception e) {
            return joinPoint.proceed();
        }
    }

//    @Before("executeManageLocationsActivity() || executeSplashRunView() || executeSplashGetRecordCount() " +
//            "|| executeRainViewer() || executeChildA() || executeChildB() || executeChildC()")
//    @Before("executeRainViewer() || executeChildA() || executeChildB() || executeChildC()")
//    @Before("executeCfgChildA() || executeCfgChildB() || executeCfgChildC()")
//    public void weaveJoinPoint(JoinPoint joinPoint) throws Throwable {
//        if (joinPoint == null) {
//            return;
//        }
//        MethodSignature methodSignature = (MethodSignature) joinPoint.getSignature();
//        methodIdMap.putIfAbsent(methodSignature.toString(), methodIdMap.size());
//
//        long startFd = fd > 0 ? readAshMem(fd) : -1;
////        long endFd = fd > 0 ? readAshMem(fd) : -1;
//        long endFd = startFd;
//        MethodStat methodStat = new MethodStat(methodIdMap.get(methodSignature.toString()), startFd, endFd);
////        Log.d("#Aspect ", methodSignature.toString()+" "+startFd+" "+endFd);
//        insert_locker.lock();
//        if (methodStats.isEmpty()) {
//            methodStats.add(methodStat);
//
//        } else if (!methodStats.get(methodStats.size() - 1).equals(methodStat)) {
//            methodStats.add(methodStat);
//        }
//        insert_locker.unlock();
//
//    }

    public static native long readAshMem1(int fd);
}
