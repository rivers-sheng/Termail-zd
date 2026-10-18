package com.termail.terminal;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.IBinder;
import android.os.PowerManager;

public class TermailService extends Service {
    private static final String CHANNEL_ID = "termail_service";
    private static final int NOTIFICATION_ID = 1001;
    private PowerManager.WakeLock wakeLock;
    private static boolean isWakeLockOn = false;

    @Override
    public void onCreate() {
        super.onCreate();
        createNotificationChannel();
        startForeground(NOTIFICATION_ID, buildNotification());
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent != null && intent.getAction() != null) {
            String action = intent.getAction();
            if ("STOP".equals(action)) {
                stopSelf();
                return START_NOT_STICKY;
            } else if ("TOGGLE_WAKELOCK".equals(action)) {
                toggleWakeLock();
                updateNotification();
                return START_STICKY;
            }
        }
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (wakeLock != null && wakeLock.isHeld()) {
            wakeLock.release();
        }
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                CHANNEL_ID, "Termail 服务",
                NotificationManager.IMPORTANCE_LOW);
            channel.setDescription("Termail 终端运行通知");
            NotificationManager mgr = getSystemService(NotificationManager.class);
            if (mgr != null) mgr.createNotificationChannel(channel);
        }
    }

    private Notification buildNotification() {
        Intent mainIntent = new Intent(this, org.qtproject.qt.android.bindings.QtActivity.class);
        mainIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        PendingIntent contentIntent = PendingIntent.getActivity(
            this, 0, mainIntent,
            PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

        Intent stopIntent = new Intent(this, TermailService.class);
        stopIntent.setAction("STOP");
        PendingIntent stopPending = PendingIntent.getService(
            this, 1, stopIntent,
            PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

        Intent wakelockIntent = new Intent(this, TermailService.class);
        wakelockIntent.setAction("TOGGLE_WAKELOCK");
        PendingIntent wakelockPending = PendingIntent.getService(
            this, 2, wakelockIntent,
            PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

        String wakeText = isWakeLockOn ? "唤醒锁: 开" : "唤醒锁: 关";

        Notification.Builder builder;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            builder = new Notification.Builder(this, CHANNEL_ID);
        } else {
            builder = new Notification.Builder(this);
        }

        return builder
            .setContentTitle("Termail终端正在运行")
            .setContentText("点击打开")
            .setSmallIcon(android.R.drawable.ic_menu_manage)
            .setContentIntent(contentIntent)
            .setOngoing(true)
            .addAction(android.R.drawable.ic_delete, "停止", stopPending)
            .addAction(android.R.drawable.ic_lock_idle_lock, wakeText, wakelockPending)
            .setPriority(Notification.PRIORITY_LOW)
            .build();
    }

    private void updateNotification() {
        NotificationManager mgr = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        if (mgr != null) {
            mgr.notify(NOTIFICATION_ID, buildNotification());
        }
    }

    private void toggleWakeLock() {
        if (isWakeLockOn) {
            if (wakeLock != null && wakeLock.isHeld()) {
                wakeLock.release();
            }
            isWakeLockOn = false;
        } else {
            PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
            if (pm != null) {
                wakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "Termail::WakeLock");
                wakeLock.acquire();
                isWakeLockOn = true;
            }
        }
    }

    public static void start(Context context) {
        Intent intent = new Intent(context, TermailService.class);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            context.startForegroundService(intent);
        } else {
            context.startService(intent);
        }
    }

    public static void stop(Context context) {
        Intent intent = new Intent(context, TermailService.class);
        context.stopService(intent);
    }

    public static void toggleWakeLock(Context context) {
        Intent intent = new Intent(context, TermailService.class);
        intent.setAction("TOGGLE_WAKELOCK");
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            context.startForegroundService(intent);
        } else {
            context.startService(intent);
        }
    }

    public static boolean isWakeLockOn() {
        return isWakeLockOn;
    }
}
