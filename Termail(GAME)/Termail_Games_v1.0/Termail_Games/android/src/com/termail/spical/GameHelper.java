package com.termail.spical;

import android.content.Context;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class GameHelper {

    // 读取游戏介绍 I.txt
    public static String readGameInfo(Context context, String gameName) {
        try {
            InputStream is = context.getAssets().open("games/" + gameName + "/I.txt");
            BufferedReader reader = new BufferedReader(new InputStreamReader(is, "UTF-8"));
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) {
                sb.append(line).append("\n");
            }
            reader.close();
            is.close();
            return sb.toString();
        } catch (Exception e) {
            return "暂无介绍";
        }
    }

    // 列出所有游戏目录
    public static String[] listGames(Context context) {
        try {
            String[] dirs = context.getAssets().list("games");
            if (dirs == null) return new String[0];
            return dirs;
        } catch (Exception e) {
            return new String[0];
        }
    }

    // 检查游戏是否存在
    public static boolean gameExists(Context context, String gameName) {
        try {
            String[] files = context.getAssets().list("games/" + gameName);
            if (files == null) return false;
            for (String f : files) {
                if (f.equals("index.html")) return true;
            }
            return false;
        } catch (Exception e) {
            return false;
        }
    }
}