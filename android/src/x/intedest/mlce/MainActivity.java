package x.intedest.mlce;

import org.libsdl.app.SDLActivity;
import android.os.Bundle;
import android.content.res.AssetManager;
import android.util.Log;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class MainActivity extends SDLActivity {
    private static final String TAG = "MLCE";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        File targetDir = getFilesDir();
        try {
            // 1. Копируем всю папку Common (включая Common/Media/MediaWindows64.arc)
            copyAssetFolder(getAssets(), "Common", targetDir.getAbsolutePath() + "/Common");
            
            // 2. Копируем любые файлы (.arc, .json, .txt) из корня ассетов в files/ (если они там окажутся)
            String[] files = getAssets().list("");
            if (files != null) {
                for (String file : files) {
                    if (file.endsWith(".arc") || file.endsWith(".json") || file.endsWith(".txt")) {
                        copyAssetFile(getAssets(), file, targetDir.getAbsolutePath() + "/" + file);
                    }
                }
            }

            // 3. Создаем символические ссылки для поддержки кроссплатформенных путей C++ кода.
            // Напрямую нацеливаемся на оригинальный MediaWindows64.arc во избежание рекурсивных петель линковки.
            File mediaDir = new File(targetDir, "Common/Media");
            if (mediaDir.exists()) {
                File realArc = new File(mediaDir, "MediaWindows64.arc");
                if (realArc.exists()) {
                    String target = realArc.getAbsolutePath();
                    
                    createSymlink(target, new File(targetDir, "Common/Media/MediaLinux.arc").getAbsolutePath());
                    createSymlink(target, new File(targetDir, "app/common/Media/MediaWindows64.arc").getAbsolutePath());
                    createSymlink(target, new File(targetDir, "app/common/Media/MediaLinux.arc").getAbsolutePath());
                }
            }
            
            System.setProperty("user.dir", targetDir.getAbsolutePath());
        } catch (Exception e) {
            Log.e(TAG, "Failed to prepare assets", e);
        }

        super.onCreate(savedInstanceState);
    }

    private static void createSymlink(String targetPath, String linkPath) {
        // Защита от зацикливания на самом себе
        if (targetPath.equals(linkPath)) {
            return;
        }
        File linkFile = new File(linkPath);
        if (linkFile.exists()) {
            linkFile.delete();
        }
        // Создаем родительские директории, если их нет
        linkFile.getParentFile().mkdirs();
        try {
            android.system.Os.symlink(targetPath, linkPath);
            Log.i(TAG, "Symlinked: " + linkPath + " -> " + targetPath);
        } catch (Exception e) {
            Log.e(TAG, "Failed to symlink " + targetPath + " to " + linkPath + ", falling back to copy", e);
            try {
                copyFile(new File(targetPath), linkFile);
            } catch (IOException ioException) {
                Log.e(TAG, "Fallback copy failed", ioException);
            }
        }
    }

    private static void copyFile(File src, File dst) throws IOException {
        try (InputStream in = new java.io.FileInputStream(src);
             OutputStream out = new FileOutputStream(dst)) {
            byte[] buf = new byte[16384];
            int len;
            while ((len = in.read(buf)) > 0) {
                out.write(buf, 0, len);
            }
        }
    }

    private static void copyAssetFolder(AssetManager am, String from, String to) throws IOException {
        File toDir = new File(to);
        if (!toDir.exists()) {
            toDir.mkdirs();
        }
        String[] files = am.list(from);
        if (files == null) return;

        for (int i = 0; i < files.length; i++) {
            String file = files[i];
            String assetFile = from.isEmpty() ? file : from + "/" + file;
            String destFile = to + "/" + file;
            String[] subFiles = am.list(assetFile);

            if (subFiles != null && subFiles.length > 0) {
                copyAssetFolder(am, assetFile, destFile);
            } else {
                copyAssetFile(am, assetFile, destFile);
            }
        }
    }

    private static void copyAssetFile(AssetManager am, String fromAssetPath, String toPath) throws IOException {
        File outFile = new File(toPath);
        if (outFile.exists() && outFile.length() > 0) {
            return;
        }

        InputStream in = null;
        OutputStream out = null;
        try {
            in = am.open(fromAssetPath);
            out = new FileOutputStream(outFile);
            byte[] buffer = new byte[16384];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
        } finally {
            if (in != null) {
                try { in.close(); } catch (IOException e) {}
            }
            if (out != null) {
                try { out.close(); } catch (IOException e) {}
            }
        }
    }

    @Override
    protected String[] getLibraries() {
        return new String[] {
            "c++_shared",
            "SDL2",
            "main"
        };
    }
}