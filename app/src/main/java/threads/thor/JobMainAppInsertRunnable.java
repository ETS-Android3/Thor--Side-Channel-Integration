package threads.thor;

import static threads.thor.MainActivity.methodStats;

import android.content.ContentValues;
import android.content.Context;
import android.database.sqlite.SQLiteDatabase;

import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;


public class JobMainAppInsertRunnable implements Runnable {
    Context context;
    SQLiteDatabase db;
    ContentValues values;
    public static Lock insert_locker = new ReentrantLock();

    private static final String TAG = "JobInsertRunnable";
    /**
     * Constructor for this class

     * @param context:           Android activity context for opening the database
     */
    public JobMainAppInsertRunnable(Context context) {
        this.context = context;
    }

    /**
     * Method to perform the operation in a different thread (from the Runnable interface)
     */
    public void run() {
        // Start timing the entire process and open the database
        insert_locker.lock();//locked here, in case that other thread delete the ArrayList
        if(methodStats.isEmpty()){
            insert_locker.unlock();
            return;
        }
        db = context.openOrCreateDatabase("MainApp.db", Context.MODE_PRIVATE, null);
        // DB transaction for faster batch insertion of data into database
        db.beginTransaction();
        synchronized (methodStats) {
            // Ground Truth insertion
            if (!methodStats.isEmpty()) {
                values = new ContentValues();
                for (threads.thor.MethodStat methodStat : methodStats) {
                    values.put(threads.thor.SideChannelContract.Columns.METHOD_ID,
                            methodStat.getId());
                    values.put(threads.thor.SideChannelContract.Columns.START_COUNT,
                            methodStat.getStartCount());
                    values.put(threads.thor.SideChannelContract.Columns.END_COUNT,
                            methodStat.getEndCount());
                    long out = db.insert(threads.thor.SideChannelContract.GROUND_TRUTH_AOP, null, values);

                }
//            Log.d(TAG+" mainAppDb #4_1# ", "" + groundTruthValues.size());


            }
            methodStats.clear();
        }



        db.setTransactionSuccessful();
        db.endTransaction();
        db.close();
        insert_locker.unlock();
        boolean ifcompress = threads.thor.Utils.checkfile(context);//get the size of db
        if(ifcompress) {//if the db is larger than limit size, compress it.
            threads.thor.Utils.compress(context);
        }
//        Log.d(TAG, "Time taken for DB storage (ms): " + deltaTime);
    }
}

