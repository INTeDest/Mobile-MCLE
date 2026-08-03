#!/bin/bash
set -ex

# ================= 1. НАСТРОЙКИ И ПУТИ =================
API=29
ARCH="arm64-v8a"
TARGET="aarch64-linux-android"

SDK_DIR=$ANDROID_SDK_ROOT
NDK_DIR=$ANDROID_NDK_ROOT
TOOLCHAIN="$NDK_DIR/toolchains/llvm/prebuilt/linux-x86_64"

BUILD_TOOLS_VERSION="34.0.0"
PLATFORM_JAR="$SDK_DIR/platforms/android-34/android.jar"
BUILD_TOOLS="$SDK_DIR/build-tools/$BUILD_TOOLS_VERSION"

AAPT="$BUILD_TOOLS/aapt"
D8="$BUILD_TOOLS/d8"
ZIPALIGN="$BUILD_TOOLS/zipalign"
APKSIGNER="$BUILD_TOOLS/apksigner"

export PATH="$TOOLCHAIN/bin:$PATH"

# ================= 2. СКАЧИВАНИЕ ECJ (Java Compiler) =================
if [ ! -f "ecj.jar" ]; then
    wget -q -O ecj.jar https://repo1.maven.org/maven2/org/eclipse/jdt/ecj/3.33.0/ecj-3.33.0.jar
fi

# ================= 3. СБОРКА СВЕЖЕГО SDL2 =================
if [ ! -d "sdl_install" ]; then
    git clone -b SDL2 https://github.com/libsdl-org/SDL.git sdl_src || true
    mkdir -p sdl_build && cd sdl_build

    cmake ../sdl_src \
        -DCMAKE_TOOLCHAIN_FILE="$NDK_DIR/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI=$ARCH \
        -DANDROID_PLATFORM=android-$API \
        -DCMAKE_INSTALL_PREFIX="$(pwd)/../sdl_install" \
        -DSDL_SHARED=ON \
        -DSDL_STATIC=OFF \
        -DSDL_TEST=OFF

    make -j$(nproc)
    make install
    cd ..
fi

# ================= 4. НАСТРОЙКА PKG-CONFIG =================
export PKG_CONFIG_PATH="$(pwd)/sdl_install/lib/pkgconfig:$(pwd)/scripts/android_pkgconfig"

# ================= 5. СБОРКА C++ ЧАСТИ С MESON =================
rm -rf build-android
meson setup build-android \
    --cross-file scripts/cross_android_aarch64.ini \
    -Drenderer=gles \
    -Dui_backend=java \
    --buildtype=release

meson compile -C build-android

# ================= 6. ПОДГОТОВКА APK РАБОЧЕЙ ОБЛАСТИ =================
rm -rf apk_workspace app-unsigned.apk debug.apk
mkdir -p apk_workspace/{lib/$ARCH,assets,java_src,obj}

# Нативные библиотеки
find build-android -name "libmain.so" -exec cp {} apk_workspace/lib/$ARCH/ \;
find sdl_install/lib -name "libSDL2.so" -exec cp {} apk_workspace/lib/$ARCH/ \;
cp "$TOOLCHAIN/sysroot/usr/lib/$TARGET/libc++_shared.so" apk_workspace/lib/$ARCH/

# Игровые ассеты (папка Common)
cp -r build-android/targets/app/Common apk_workspace/assets/

# Копируем любые файлы (.arc, .json, .txt) из корня targets/app в корень ассетов APK
find build-android/targets/app -maxdepth 1 -type f ! -name "*.so" ! -name "*.stamp" -exec cp {} apk_workspace/assets/ \;

# Java исходники SDL
cp -r sdl_src/android-project/app/src/main/java/org apk_workspace/java_src/

# Восстанавливаем/копируем MainActivity.java
mkdir -p apk_workspace/java_src/x/intedest/mlce
if [ -f "MainActivity.java" ]; then
    cp MainActivity.java apk_workspace/java_src/x/intedest/mlce/
elif [ -f "android/src/x/intedest/mlce/MainActivity.java" ]; then
    cp android/src/x/intedest/mlce/MainActivity.java apk_workspace/java_src/x/intedest/mlce/
else
    # Создаем резервный MainActivity.java, если файл потерялся
    cat << 'EOF' > apk_workspace/java_src/x/intedest/mlce/MainActivity.java
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
            copyAssetFolder(getAssets(), "Common", targetDir.getAbsolutePath() + "/Common");
            
            String[] files = getAssets().list("");
            if (files != null) {
                for (String file : files) {
                    if (file.endsWith(".arc") || file.endsWith(".json") || file.endsWith(".txt")) {
                        copyAssetFile(getAssets(), file, targetDir.getAbsolutePath() + "/" + file);
                    }
                }
            }
            
            System.setProperty("user.dir", targetDir.getAbsolutePath());
        } catch (Exception e) {
            Log.e(TAG, "Failed to prepare assets", e);
        }

        super.onCreate(savedInstanceState);
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
EOF
fi

# Копируем из android/src если папка существует
if [ -d "android/src" ]; then
    if [ "$(ls -A android/src 2>/dev/null)" ]; then
        cp -r android/src/* apk_workspace/java_src/
    fi
fi

# ================= 7. КОМПИЛЯЦИЯ JAVA В DEX =================
java -jar ecj.jar -1.8 -target 1.8 -d apk_workspace/obj -classpath "$PLATFORM_JAR" $(find apk_workspace/java_src -name "*.java")

# Выводим classes.dex прямо в корень apk_workspace, чтобы он запаковался по правильному пути!
$D8 --release --min-api 21 --lib "$PLATFORM_JAR" --output apk_workspace/ $(find apk_workspace/obj -name "*.class")

# ================= 8. СБОРКА И ПОДПИСЬ APK =================

# Настройка AndroidManifest.xml
MANIFEST="apk_workspace/AndroidManifest.xml"
if [ -f "AndroidManifest.xml" ]; then
    cp AndroidManifest.xml "$MANIFEST"
elif [ -f "android/AndroidManifest.xml" ]; then
    cp android/AndroidManifest.xml "$MANIFEST"
else
    # Генерируем резервный AndroidManifest.xml, если он потерялся
    cat << 'EOF' > "$MANIFEST"
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="x.intedest.mlce"
    android:versionCode="1"
    android:versionName="0.1.0">

    <uses-feature android:glEsVersion="0x00030000" android:required="true" />
    <uses-permission android:name="android.permission.INTERNET"/>

    <application
        android:allowBackup="true"
        android:label="@string/app_name"
        android:hasFragileUserData="false"
        android:hardwareAccelerated="true">

        <activity android:name="x.intedest.mlce.MainActivity"
                  android:theme="@android:style/Theme.NoTitleBar.Fullscreen"
                  android:screenOrientation="landscape"
                  android:configChanges="keyboardHidden|orientation|screenSize"
                  android:exported="true">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>
</manifest>
EOF
fi

# Настройка ресурсов (папка res)
RES_DIR="apk_workspace/res"
mkdir -p "$RES_DIR/values"

if [ -d "android/res" ] && [ "$(ls -A android/res 2>/dev/null)" ]; then
    cp -r android/res/* "$RES_DIR/"
elif [ -d "res" ] && [ "$(ls -A res 2>/dev/null)" ]; then
    cp -r res/* "$RES_DIR/"
fi

# Если strings.xml все еще отсутствует, создаем дефолтный, чтобы AAPT не ругался на @string/app_name
if [ ! -f "$RES_DIR/values/strings.xml" ]; then
    cat << 'EOF' > "$RES_DIR/values/strings.xml"
<?xml version="1.0" encoding="utf-8"?>
<resources>
    <string name="app_name">Native MLCE</string>
</resources>
EOF
fi

# Упаковываем ассеты и ресурсы
$AAPT package -f -M "$MANIFEST" -I "$PLATFORM_JAR" -S "$RES_DIR" -A apk_workspace/assets -F app-unsigned.apk

# Добавляем в APK нативные либы и скомпилированный dex из корня рабочей области
cd apk_workspace
zip -r ../app-unsigned.apk lib/ classes.dex
cd ..

# Генерируем ключ подписи, если его нет
if [ ! -f "debug.keystore" ]; then
    keytool -genkey -v -keystore debug.keystore -storepass android -alias androiddebugkey -keypass android -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=Android Debug,O=Android,C=US"
fi

$ZIPALIGN -f -p 4 app-unsigned.apk debug.apk
$APKSIGNER sign --ks debug.keystore --ks-pass pass:android debug.apk

echo "Сборка завершена успешно! Итоговый файл: debug.apk"