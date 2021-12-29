package threads.thor;

import static threads.thor.MainActivity.groundTruthValues;
import static threads.thor.MainActivity.sideChannelValues;

import android.content.ContentValues;
import android.content.Context;
import android.database.sqlite.SQLiteDatabase;

import java.util.ArrayList;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class JobInsertRunnable implements Runnable {
    Context context;
    SQLiteDatabase db;
    ContentValues values;
    long startTime;
    public static Lock insert_locker = new ReentrantLock();
    public static Lock aspect_insert_locker = new ReentrantLock();

    private static final String TAG = "JobInsertRunnable";
    /**
     * Constructor for this class

     * @param context:           Android activity context for opening the database
     */
    public JobInsertRunnable(Context context) {
        this.context = context;
        /*
        this.groundTruthValues = groundTruthValues;
        this.userFeedbacks = userFeedbacks;
        this.compilerValues = compilerValues;
        this.frontAppValues = frontAppValues;

         */
    }

    /**
     * Method to perform the operation in a different thread (from the Runnable interface)
     */
    public void run() {
        // Start timing the entire process and open the database
        insert_locker.lock();//locked here, in case that other thread delete the ArrayList
        if(sideChannelValues.isEmpty() && groundTruthValues.isEmpty()){
            insert_locker.unlock();
            return;
        }
        startTime = System.currentTimeMillis();
        db = context.openOrCreateDatabase("SideScan.db", Context.MODE_PRIVATE, null);
        // DB transaction for faster batch insertion of data into database
        db.beginTransaction();
//        Log.d("sidescandb", "sideChannelValues "+sideChannelValues.size()+ " groundTruthValues "+groundTruthValues.size() );
//        if(!sideChannelValues.isEmpty()){
//            Log.d("sidescandb", "sideChannelValues[0] "+sideChannelValues.get(0).getCount()+ " | "+ sideChannelValues.get(0).getAddress());
//        }
//        if(!groundTruthValues.isEmpty()){
//            Log.d("sidescandb", "groundTruthValues[0] "+groundTruthValues.get(0).getCount());
//        }

        if(sideChannelValues!=null&&sideChannelValues.size()!=0) {
            values = new ContentValues();
            for (threads.thor.SideChannelValue sideChannelValue:sideChannelValues) {
                values.put(threads.thor.SideChannelContract.Columns.SYSTEM_TIME,
                        sideChannelValue.getSystemTime());
                values.put(threads.thor.SideChannelContract.Columns.COUNT,
                        sideChannelValue.getCount());
                values.put(threads.thor.SideChannelContract.Columns.SCAN_TIME,
                        sideChannelValue.getScanTime());
                values.put(threads.thor.SideChannelContract.Columns.ADDRESS,
                        sideChannelValue.getAddress());

                long out =  db.insert(threads.thor.SideChannelContract.TABLE_NAME, null, values);
//                Log.d(TAG +" sidescandb ", "SideChannelContract.GROUND_TRUTH " + out);

            }
            sideChannelValues = new ArrayList<>();
        }
//        Log.d("sidescandb", "groundTruthValues "+groundTruthValues.size());

        db.setTransactionSuccessful();
        db.endTransaction();
        db.close();
        long deltaTime = System.currentTimeMillis() - startTime;
        insert_locker.unlock();
        boolean ifcompress = threads.thor.Utils.checkfile(context);//get the size of db
        if(ifcompress) {//if the db is larger than limit size, compress it.
            threads.thor.Utils.compress(context);
        }
//        Log.d(TAG, "Time taken for DB storage (ms): " + deltaTime);
    }
}

