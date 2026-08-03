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
mkdir -p android_pkgconfig

cat <<EOF > android_pkgconfig/zlib.pc
Name: zlib
Description: zlib for Android NDK
Version: 1.3
Libs: -lz
Cflags:
EOF

export PKG_CONFIG_PATH="$(pwd)/sdl_install/lib/pkgconfig:$(pwd)/android_pkgconfig"

# ================= 5. СБОРКА C++ ЧАСТИ С MESON =================
rm -rf build-android
meson setup build-android \
    --cross-file scripts/cross_android_aarch64.ini \
    -Drenderer=gles \
    -Dui_backend=java \
    --buildtype=release

meson compile -C build-android

# ================= 6. ПОДГОТОВКА APK РАБОЧЕЙ ОБЛАСТИ =================
rm -rf apk_workspace app-unsigned.apk 4jcraft-debug.apk
mkdir -p apk_workspace/{lib/$ARCH,assets,java_src,obj,dex}

# Нативные библиотеки
find build-android -name "libmain.so" -exec cp {} apk_workspace/lib/$ARCH/ \;
find sdl_install/lib -name "libSDL2.so" -exec cp {} apk_workspace/lib/$ARCH/ \;
cp "$TOOLCHAIN/sysroot/usr/lib/$TARGET/libc++_shared.so" apk_workspace/lib/$ARCH/

# Игровые ассеты
cp -r build-android/targets/app/Common apk_workspace/assets/

# Java исходники
cp -r android/src/* apk_workspace/java_src/
cp -r sdl_src/android-project/app/src/main/java/org apk_workspace/java_src/

# ================= 7. КОМПИЛЯЦИЯ JAVA В DEX =================
java -jar ecj.jar -1.8 -target 1.8 -d apk_workspace/obj -classpath "$PLATFORM_JAR" $(find apk_workspace/java_src -name "*.java")

$D8 --release --min-api 21 --lib "$PLATFORM_JAR" --output apk_workspace/dex/ $(find apk_workspace/obj -name "*.class")

# ================= 8. СБОРКА И ПОДПИСЬ APK =================
$AAPT package -f -M android/AndroidManifest.xml -I "$PLATFORM_JAR" -S android/res -A apk_workspace/assets -F app-unsigned.apk

cd apk_workspace
zip -r ../app-unsigned.apk lib/ dex/classes.dex
cd ..

keytool -genkey -v -keystore debug.keystore -storepass android -alias androiddebugkey -keypass android -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=Android Debug,O=Android,C=US"

$ZIPALIGN -f -p 4 app-unsigned.apk debug.apk
$APKSIGNER sign --ks debug.keystore --ks-pass pass:android debug.apk

echo "Сборка завершена успешно! Итоговый файл: debug.apk"