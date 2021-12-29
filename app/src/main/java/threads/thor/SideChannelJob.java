/*
This is a foreground service for scanning, side channel information collection and notificaiton poping. 
*/
package threads.thor;

import static threads.thor.MainActivity.PACKAGE_NAME;
import static threads.thor.MainActivity.cs;
import static threads.thor.MainActivity.methodStats;
import static threads.thor.MainActivity.sideChannelValues;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.DatabaseUtils;
import android.database.sqlite.SQLiteDatabase;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.ParcelFileDescriptor;
import android.os.Process;
import android.os.RemoteException;
import android.util.Log;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;

import java.io.IOException;
import java.math.BigInteger;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.StringTokenizer;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

//import static CacheScan.answered;
//import static CacheScan.notification;
//import static JobInsertRunnable.insert_locker;


public class SideChannelJob extends Service {
    public static volatile boolean continueRun;
    private static final String TAG = "AddressScan";
    private static int label = 0;
    static int[] pattern_filter = null;
    int scValueCount = 1;
    int index = 1;
    long cumulativeTime;
    private String currentFront = "DevSec";
    private static final String CHANNEL_ID = "e.smu.devsec";
    private static final String description = "Collecting Data";
    static Lock locker = new ReentrantLock();
    static String shared_mem_des = "";
    long start_time = 0;
    Thread thread_collect = null;
    Thread thread_notify = null;

    static long localCount = 0;
    //    don't change the format
    static int yieldVal = 100;

    static int fd = -2;

    Map<String, String> configMap = new HashMap<>();
    Map<String, String> toolConfigMap = new HashMap<>();
    static final String CONFIG_FILE_PATH = "/data/local/tmp/config.out";
    static final String TOOL_CONFIG_FILE_PATH = "/data/local/tmp/toolConfig.out";

    /**
     * Start service by notification
     */
    @TargetApi(Build.VERSION_CODES.N)
    public void setForegroundService() {
        int importance = NotificationManager.IMPORTANCE_LOW;
        //build channel
        NotificationChannel channel = new NotificationChannel(CHANNEL_ID, CHANNEL_ID, importance);
        Intent intent;
        channel.setDescription(description);
        intent = new Intent(this, MainActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent, 0);
        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, CHANNEL_ID);
        builder.setSmallIcon(R.mipmap.ic_launcher_round)
                .setContentTitle("DevSec")
                .setContentText("Scanning")
                .setOngoing(true)
                .setContentIntent(pendingIntent)
                .setDeleteIntent(pendingIntent);
        NotificationManager notificationManager = getSystemService(NotificationManager.class);
        notificationManager.createNotificationChannel(channel);
        startForeground(111, builder.build());
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG, "Job started");
        start_time = System.currentTimeMillis();

        initializeDB();
        configMap = readConfigFile(CONFIG_FILE_PATH);
        toolConfigMap = readConfigFile(TOOL_CONFIG_FILE_PATH);
        toolConfigMap.entrySet().forEach(e -> Log.d("toolConfigMap: ", e.getKey() + " " + e.getValue()));
        yieldVal = Integer.parseInt(Objects.requireNonNull(configMap.get("yield")));
        doBackgroundWork();
        Toast.makeText(this, "Job scheduled successfully", Toast.LENGTH_SHORT)
                .show();
        return super.onStartCommand(intent, flags, startId);
    }

    /**
     * Method to run data collection in the background
     */
    public void doBackgroundWork() {
        Log.d(TAG, "New Thread Created");

        Thread ashMemUpdatorThread = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    while (fd < 0) {
                        Thread.sleep(10);
                    }
                    int i = 1;
                    long j = 0l;
                    while (true) {
                        i = 1;
                        j = j + 1;
                        setAshMemVal(fd, j);
                        while (i % 100 != 0) {
                            i++;
                        }

                    }
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        });

//         Send the job to a different thread
        new Thread(new Runnable() {
            @Override
            public void run() {
                BigInteger odexMemMapBegin;
                BigInteger odexMemMapBegin2;

                try {
                    while (fd < 0) {
                        Thread.sleep(10);
                    }
                    if (cs == null) {

                        cs = new threads.thor.CacheScan(getBaseContext());//Initialize the CacheScan class


                        Log.d(TAG, "Initialize CacheScan");
//                        setSharedMapChild(2, shared_mem_des.toCharArray());

////                        exec memory
                        String odexExLine = getOdexExLine();
                        BigInteger odexMemExEnd = new BigInteger(odexExLine.split("-")[1], 16);
                        BigInteger odexMemExBegin = new BigInteger(odexExLine.split("-")[0], 16);


                        Log.d("OdexScan::", "exec memory begin: " + odexMemExBegin.toString(16));
                        Log.d("OdexScan::", "exec memory end: " + odexMemExEnd.toString(16));


////                        exec offset
                        String execOffsetString = configMap.get("execOffset");
                        BigInteger execOffset = new BigInteger(execOffsetString, 16);

                        Log.d("OdexScan::", "exec offset: " + execOffset.toString(16));

                        odexMemMapBegin = new BigInteger(getOdexBeginAddress(), 16);
                        odexMemMapBegin = odexMemMapBegin.add(new BigInteger("1000", 16));
                        Log.d("AddressComb", "base address start: " + odexMemMapBegin.toString(16) + "|" + odexMemMapBegin.toString(10));
                        Log.d("AddressComb", "exec memory begin: " + odexMemExBegin.toString(16) + "|" + odexMemExBegin.toString(10));
                        Log.d("AddressComb", "exec memory end: " + odexMemExEnd.toString(16) + "|" + odexMemExEnd.toString(10));
                        Log.d("AddressComb", "exec offset: " + execOffset.toString(16));


//##########

                        String addressForAdjusting = configMap.get("addressForAdjusting");
                        String[] adjustingAddressesString = addressForAdjusting.split(",");
                        long[] adjustingAddressesLong = new long[adjustingAddressesString.length];
                        for (int i = 0; i < adjustingAddressesString.length; i++) {
//                          (exec memory-exec offset)+code offset
                            adjustingAddressesString[i] = odexMemMapBegin
                                    .add(new BigInteger(adjustingAddressesString[i], 16)).toString(10);
                            adjustingAddressesLong[i] = Long.parseLong(adjustingAddressesString[i]);
                        }
                        if (("1").equals(configMap.get("isAdjust"))) {
                            adjustThreshold(adjustingAddressesLong, adjustingAddressesLong.length);
                        }


//##########
                        String[] offsets = configMap.get("sideChannelOffsets").split(",");
                        Log.d("offsets in hex:", String.join(",", Arrays.asList(offsets)));
                        int pauseVal = Integer.parseInt(configMap.get("pauseVal").trim());
                        int hitVal = Integer.parseInt(configMap.get("hitVal").trim());
                        boolean resetHitCounter = "1".equals(configMap.get("resetHitCounter").trim());
                        int splitVal = Integer.parseInt(configMap.get("splitVal").trim());
                        String[] methodNames = toolConfigMap.get("methodIdNameMapping").split("\\|");
//don't change


//                        BigInteger o1 = new BigInteger(offsets[0], 16);
//                        BigInteger o2 = new BigInteger(offsets[2], 16);
//                        BigInteger o3 = new BigInteger(offsets[3], 16);
//
//                        BigInteger basePlusExecOffset = odexMemMapBegin.add(execOffset);
//                        Log.d("AddressComb", "base address + exec offset " + odexMemMapBegin.toString(16) + "|" + odexMemMapBegin.toString(10));
//                        Log.d("AddressComb", basePlusExecOffset.add(o1).toString(16)
//                                + "," + basePlusExecOffset.add(o2).toString(16)
//                                + "," + basePlusExecOffset.add(o3).toString(16));
//
//                        BigInteger ExecStartPlusExecOffset = odexMemExBegin.add(execOffset);
//                        Log.d("AddressComb", "executable region start address + exec offset " + ExecStartPlusExecOffset.toString(16) + "|"
//                                + ExecStartPlusExecOffset.toString(10));
//                        Log.d("AddressComb", ExecStartPlusExecOffset.add(o1).toString(16)
//                                + "," + ExecStartPlusExecOffset.add(o2).toString(16)
//                                + "," + ExecStartPlusExecOffset.add(o3).toString(16));
//
//                        BigInteger execStartMinusExecOffset = odexMemExBegin.subtract(execOffset);
//                        Log.d("AddressComb", "executable region start address - exec offset " + execStartMinusExecOffset.toString(16) + "|"
//                                + execStartMinusExecOffset.toString(10));
//                        Log.d("AddressComb", execStartMinusExecOffset.add(o1).toString(16)
//                                + "," + execStartMinusExecOffset.add(o2).toString(16)
//                                + "," + execStartMinusExecOffset.add(o3).toString(16));


                        long[] longOffsets = new long[offsets.length];
                        List<Integer> splitIndexes = new ArrayList<>();
                        splitIndexes.add(0);
                        IntStream.range(1, 1 + (longOffsets.length / splitVal))
                                .forEach(b -> splitIndexes.add(Math.min(b * splitVal, longOffsets.length)));
                        Log.d("splitIndexes", splitIndexes.toString());
                        for (int i = 0; i < offsets.length; i++) {

                            offsets[i] = odexMemMapBegin.add(new BigInteger(offsets[i], 16))
                                    .toString(10);
                            if (odexMemExEnd.compareTo(new BigInteger(offsets[i], 10)) > 0) {
                                longOffsets[i] = Long.parseLong(offsets[i]);
                            } else {
                                Log.d("odex ", offsets[i] + " | " + new BigInteger(offsets[i], 10).toString(16) + " is out of range");
                            }
//                            intOffsets[i] = Integer.parseInt(offsets[i]);
                        }
                        Log.d("odex", " end of for");
                        Log.d(TAG, "odex scanned offsets:" + String.join(",", offsets));
                        Log.d(TAG, " offsetsWithNames:\n" + IntStream.range(0, methodNames.length).mapToObj(i -> methodNames[i] + "->" + offsets[i + 1]).map(String::valueOf).collect(Collectors.joining("\n")));

                        ashMemUpdatorThread.start();
                        while (true) {

                            localCount++;
                            if (localCount % yieldVal == 0) {
                                localCount = 0;
                                for (int sp = 0; sp < splitIndexes.size() - 1; sp++) {
                                    scan7(Arrays.copyOfRange(longOffsets, splitIndexes.get(sp), splitIndexes.get(sp + 1))
                                            , splitIndexes.get(sp + 1) - splitIndexes.get(sp), pauseVal, hitVal, resetHitCounter);
                                }
                                if (fd < 0) {
                                    Log.d("ashmem ", "not set yet" + fd);
                                } else {
//                                    setAshMemVal(fd, timeCount);
                                }
                                if ("1".equals(configMap.get("sideChannelSleep"))) {
                                    Thread.sleep(1);
                                }

                            }

                        }
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                }
                //Start Scaning
                if (pattern_filter == null)
                    pattern_filter = Utils.getArray(getBaseContext(), "ptfilter");//Retrieve an array of pattern filter; since we only need to monitor partial functions, others will get blocked
            }
        }).start();


        thread_collect = new Thread(new Runnable() {
            @Override
            public void run() {
                Log.d("sidescandb", "thread_collect run");

                // Initializing primitives and objects
                // Loop to collect side channel values via API calls
                try {
                    while (true) {

                        Thread.sleep(100);
                        while (cs == null) {
                            Thread.sleep(100);//Waiting until CacheScan class is initialized
                        }

                        if (sideChannelValues.size() > 10) {//Do not save collected info when at trial mode
//                            Log.d("sidescandb", "sidechannel");
                            new Thread(new threads.thor.JobInsertRunnable(getBaseContext())).start();//start a thread to save data
//                            Log.d(TAG, "DB Updated");
                            index = 1;
                            scValueCount = 0;
                            cumulativeTime = 0;
                        }

                        if (methodStats.size() > 5) {
                            new Thread(new JobMainAppInsertRunnable(getBaseContext())).start();
                        }
                    }//while
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        });
        thread_collect.start();


        //Start a notification thread.
        thread_notify = new Thread(new Runnable() {
            @Override
            public void run() {
                Log.d("sidescandb", "thread_notify run");
                try {
                    while (cs == null) {
                        Thread.sleep(100);//Waiting until CacheScan class is initialized
                    }
                    while (true) {
                        Thread.sleep(2000);
                        cs.Notify(getBaseContext());//every 1 second to check if it need to send notification
                    }
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        });
        thread_notify.start();


    }

    public String getOdexBeginAddress() {

        // get Process ID of the running app
        int pid = Process.myPid();

        Log.d(TAG, "pid: " + pid);

        try {

//            printing for so
            String mapString = Files.lines(Paths.get("/proc/self/maps")).collect(Collectors.joining("\n"));
            Log.d(".so", mapString);


            Optional<String> odc = Files.lines(Paths.get("/proc/self/maps")).collect(Collectors.toList())
                    .stream().sequential().filter(s -> s.contains(PACKAGE_NAME) && s.contains("base.odex"))
                    .findFirst().map(s -> new StringTokenizer(s, "-")).filter(StringTokenizer::hasMoreElements)
                    .map(StringTokenizer::nextToken);
            Log.d(TAG, "%OdexBeginAddress " + odc);
            if (odc.isPresent()) {
                return odc.get();
            }
        } catch (Exception e) {
            Log.d(TAG, "ERROR!!!!" + e.toString());
        }
        return "0000";
    }

    public String getOdexExLine() {

        // get Process ID of the running app
        int pid = Process.myPid();
        Log.d(TAG, "pid " + pid);

        try {

            Optional<String> odc = Files.lines(Paths.get("/proc/self/maps")).collect(Collectors.toList())
                    .stream().sequential().filter(s -> s.contains(PACKAGE_NAME) && s.contains("base.odex") && s.contains("r-xp"))
                    .findFirst();

            Log.d(TAG, "odc " + odc);
            if (odc.isPresent()) {
                return odc.get().split("r-xp")[0].trim();
            }
        } catch (Exception e) {
            Log.d(TAG, "ERROR!!!!" + e.toString());
        }
        return "";
    }

    public native void scan(int[] pattern, int length);

    public native long scan7(long[] arr, int length, int pauseVal, int hitVal, boolean resetHitCounter);

    public native void adjustThreshold(long[] arr, int length);

    public static native long readAshMem(int fd);

    public static native void setAshMemVal(int fd, long val);

    public native void scanOdex(long[] arr, int length);

    public native void pause();

    public native void setSharedMapChild(int shared_mem_ptr, char[] fileDes);



    /**
     * Method to compute battery level based on API call to BatteryManager
     *
     * @param context: Android activity context for detecting change in battery charge
     * @return
     */
    private float computeBatteryLevel(Context context) {

        IntentFilter ifilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        Intent batteryStatus = context.registerReceiver(null, ifilter);

        int level = batteryStatus.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
        int scale = batteryStatus.getIntExtra(BatteryManager.EXTRA_SCALE, -1);

        return level / (float) scale;
    }

    @SuppressLint("CommitPrefEdits")
    @Override
    public void onCreate() {
        super.onCreate();
//        try {
//            int pid = android.os.Process.myPid();
//            Log.d(TAG, "%%%% spLASH! " + pid);
//            Runtime.getRuntime().exec("taskset -p f " + pid);
//            String cpuBind = getCommandResult("taskset -p " + pid);
//            Log.d(TAG, "cpu core: " + cpuBind);
//
//        } catch (Exception e) {
//            Log.d(TAG, e.toString());
//        }

        Log.d("foreground", "onCreate");
        //如果API在26以上即版本为O则调用startForefround()方法启动服务
        setForegroundService();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
//        final long minutes = (System.currentTimeMillis()-start_time)/(1000*60);
//        pause();//pause the scanning
//        //locker.lock();
//        if(stage==0) {//not in trial mode
//            if(minutes>0) {
//                new Thread(new Runnable() {
//                    @Override
//                    public void run() {
//                        String r = TimerManager.getInstance().uploadTimeCheck(getBaseContext(), minutes);//
//                        if (r != null && r.equals("1"))
//                            Log.d(TAG, "Running time are uploaded successfully");
//                        else
//                            Log.d(TAG, "Unable to upload the running time");
//                    }
//                }).start();
//            }
//        }
//        //locker.unlock();
//        SharedPreferences edit = getBaseContext().getSharedPreferences("user",0);
//        SharedPreferences.Editor editor = edit.edit();
//        editor.putLong("Answered",answered);//record the numbr of notification
//        editor.putLong("Notification",notification);
//        editor.putLong("lastday",lastday);
//        editor.putLong("day",day);
//        editor.commit();
//        continueRun = false;
//        //make sure threads are stopped
//        if(thread_collect!=null) {
//            try {
//                thread_collect.join();
//            } catch (InterruptedException e) {
//                e.printStackTrace();
//            }
//            thread_collect = null;
//        }
//        if(thread_notify!=null) {
//            try {
//                thread_notify.interrupt();
//                thread_notify.join();
//            } catch (InterruptedException e) {
//                e.printStackTrace();
//            }
//            thread_notify = null;
//        }
//        Toast.makeText(this, "Job cancelled", Toast.LENGTH_SHORT)
//                .show();
    }

    private static class MessengerHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            Log.d("ashmem", " got the message sent by the client: ");
            ParcelFileDescriptor pfd = msg.getData().getParcelable("msg");
            fd = pfd.getFd();
            if (fd < 0) {
                Log.d("ashmem ", "not set MessengerHandler " + fd);
            }
            long c = readAshMem(fd);
            setAshMemVal(fd, 5l);
            Log.d("ashmem", " got the message sent by the client: " + pfd.getFd() + " " + c);
            Log.d("ashmem", " got the message sent by the client: new val " + readAshMem(fd));

            Messenger client = msg.replyTo; // 1
            Message replyMessage = Message.obtain(null, 1);
            Bundle bundle = new Bundle();
            bundle.putString("reply", "The server has received the message!");
            replyMessage.setData(bundle);
            try {
                client.send(replyMessage);
            } catch (RemoteException e) {
                e.printStackTrace();
            }

        }
    }

    private final Messenger mMessenger = new Messenger(new MessengerHandler());

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return mMessenger.getBinder();
    }

    /**
     * Method to initialize database
     */
    void initializeDB() {
        // Creating the database file in the app sandbox
        SQLiteDatabase db = getBaseContext().openOrCreateDatabase("SideScan.db",
                MODE_PRIVATE, null);
        Locale locale = new Locale("EN", "SG");
        db.setLocale(locale);
        // Creating the schema of the database
        String sSQL = "CREATE TABLE IF NOT EXISTS " + threads.thor.SideChannelContract.TABLE_NAME + " (" +
                //SideChannelContract.Columns._ID + " INTEGER PRIMARY KEY NOT NULL, " +
                threads.thor.SideChannelContract.Columns.SYSTEM_TIME + " INTEGER NOT NULL, " +
                threads.thor.SideChannelContract.Columns.SCAN_TIME + " INTEGER NOT NULL, " +
                threads.thor.SideChannelContract.Columns.ADDRESS + " TEXT, " +
                threads.thor.SideChannelContract.Columns.COUNT + " INTEGER);";
        db.execSQL(sSQL);
        sSQL = "DELETE FROM " + threads.thor.SideChannelContract.TABLE_NAME;
        db.execSQL(sSQL);
        Log.d("dbinfo", threads.thor.SideChannelContract.TABLE_NAME + " count: " + getRecordCount(threads.thor.SideChannelContract.TABLE_NAME));

        db.close();
    }

    public long getRecordCount(String tableName) {
        SQLiteDatabase db = getBaseContext().openOrCreateDatabase("SideScan.db",
                MODE_PRIVATE, null);
        long count = DatabaseUtils.queryNumEntries(db, tableName);
        db.close();
        return count;
    }

    private Map<String, String> readConfigFile(String filePath) {
        Map<String, String> configMap = new HashMap<>();
        try {
            List<String> configs = Files.lines(Paths.get(filePath)).collect(Collectors.toList());
            configs.stream().filter(c -> !c.contains("//") && c.contains(":")).forEach(c -> configMap.put(c.split(":")[0].trim(), c.split(":")[1].trim()));
//            configMap.entrySet().forEach(e-> Log.d("configMap: " , e.getKey()+" "+e.getValue()));

        } catch (IOException e) {
            Log.d(TAG + "#", e.toString());
        }
        return configMap;
    }

}


//
////                        ####
////                        base memory
//                        odexMemMapBegin2 = new BigInteger(getOdexBeginAddress(), 16);
//                                Log.d("OdexScan::", "base memory: "+ odexMemMapBegin2.toString(16) );
////                        exec memory
//                                BigInteger odexMemExBegin = new BigInteger(getOdexBeginExAddress(), 16);
//                                Log.d("OdexScan::", "exec memory: "+ odexMemExBegin.toString(16) );
//
////                        exec offset
//                                BigInteger execOffset = new BigInteger("11a000", 16);
//                                Log.d("OdexScan::", "exec offset: "+ execOffset.toString(16) );
//
////                        code offset
//                                BigInteger codeOffset = new BigInteger("11a010", 16);
//                                Log.d("OdexScan::", "code offset: "+ codeOffset.toString(16) );
//
////                        exec memory+exec offset
//                                BigInteger odexMemEx2Begin = odexMemExBegin.add(execOffset);
//                                Log.d("OdexScan::", "exec memory+exec offset: "+ odexMemEx2Begin.toString(16) );
//
////                        exec memory+code offset
//                                BigInteger odexMemEx3Begin = odexMemExBegin.add(codeOffset);
//                                Log.d("OdexScan::", "exec memory+code offset: "+ odexMemEx3Begin.toString(16) );
//
////                        exec memory+exec offset+code offset
//                                BigInteger odexMemEx4Begin = odexMemExBegin.add(execOffset).add(codeOffset);
//                                Log.d("OdexScan::", "exec memory+exec offset+code offset: "+ odexMemEx4Begin.toString(16) );
//
////                        base memory+exec offset+code offset
//                                BigInteger odexMemEx5Begin = odexMemMapBegin2.add(execOffset).add(codeOffset);
//                                Log.d("OdexScan::", "base memory+exec offset+code offsety: "+ odexMemEx5Begin.toString(16) );
//
////                        base memory+code offset
//                                BigInteger odexMemEx6Begin = odexMemMapBegin2.add(codeOffset);
//                                Log.d("OdexScan::", "base memory+code offset: "+ odexMemEx6Begin.toString(16) );
//
////                        exec memory+code offset - exec offset
//                                BigInteger odexMemEx7Begin = odexMemExBegin.add(codeOffset).subtract(execOffset);
//                                Log.d("OdexScan::", "exec memory+code offset - exec offset: "+ odexMemEx7Begin.toString(16) );
//
//
