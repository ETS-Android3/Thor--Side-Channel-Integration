package threads.thor;

import android.app.Application;

import androidx.annotation.NonNull;

import com.google.common.reflect.TypeToken;
import com.google.gson.Gson;

import net.luminis.quic.QuicConnection;

import java.lang.reflect.Type;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;

import threads.LogUtils;
import threads.lite.IPFS;
import threads.lite.cid.PeerId;
import threads.thor.core.Content;
import threads.thor.core.DOCS;
import threads.thor.core.pages.PAGES;
import threads.thor.core.pages.Page;
import threads.thor.utils.AdBlocker;

public class InitApplication extends Application {

    public static final String TIME_TAG = "TIME_TAG";
    private static final String TAG = InitApplication.class.getSimpleName();
    private final Gson gson = new Gson();

    @Override
    public void onCreate() {
        super.onCreate();

        long start = System.currentTimeMillis();

        AdBlocker.init(getApplicationContext());


        LogUtils.info(TIME_TAG, "InitApplication after add blocker [" +
                (System.currentTimeMillis() - start) + "]...");
        try {
            IPFS ipfs = IPFS.getInstance(getApplicationContext());
            ipfs.relays();
            ipfs.setPusher(this::onMessageReceived);
        } catch (Throwable throwable) {
            LogUtils.error(TAG, throwable);
        }

        LogUtils.info(TIME_TAG, "InitApplication after starting ipfs [" +
                (System.currentTimeMillis() - start) + "]...");

    }

    @SuppressWarnings("UnstableApiUsage")
    public void onMessageReceived(@NonNull QuicConnection conn, @NonNull String content) {

        try {
            Type hashMap = new TypeToken<HashMap<String, String>>() {
            }.getType();

            Objects.requireNonNull(conn);
            IPFS ipfs = IPFS.getInstance(getApplicationContext());

            Objects.requireNonNull(content);
            Map<String, String> data = gson.fromJson(content, hashMap);

            LogUtils.debug(TAG, "Push Message : " + data.toString());


            String ipns = data.get(Content.IPNS);
            Objects.requireNonNull(ipns);
            String pid = data.get(Content.PID);
            Objects.requireNonNull(pid);
            String seq = data.get(Content.SEQ);
            Objects.requireNonNull(seq);

            PeerId peerId = PeerId.fromBase58(pid);
            long sequence = Long.parseLong(seq);
            if (sequence >= 0) {
                if (ipfs.isValidCID(ipns)) {
                    PAGES pages = PAGES.getInstance(getApplicationContext());
                    Page page = pages.createPage(peerId.toBase58());
                    page.setContent(ipns);
                    page.setSequence(sequence);
                    pages.storePage(page);
                }
            }

            DOCS.getInstance(getApplicationContext()).addResolves(peerId, ipns);

        } catch (Throwable throwable) {
            LogUtils.error(TAG, throwable);
        }
    }
}
