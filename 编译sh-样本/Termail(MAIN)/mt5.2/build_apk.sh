#!/bin/bash
export ANDROID_HOME=/home/sheng/Android/sdk
export ANDROID_NDK_ROOT=$ANDROID_HOME/ndk/25.1.8937393
export QT_ANDROID=/home/sheng/Qt6_android/6.5.0/android_arm64_v8a
export QT_HOST=/home/sheng/Qt6_host/6.5.0/gcc_64

cd /home/sheng/Termail_v5.2
rm -rf build_android
mkdir build_android && cd build_android

cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-33 \
  -DANDROID_SDK_ROOT=$ANDROID_HOME \
  -DQT_HOST_PATH=$QT_HOST \
  -DCMAKE_PREFIX_PATH=$QT_ANDROID \
  -DQt6_DIR=$QT_ANDROID/lib/cmake/Qt6 \
  -DQt6Core_DIR=$QT_ANDROID/lib/cmake/Qt6Core \
  -DQt6Gui_DIR=$QT_ANDROID/lib/cmake/Qt6Gui \
  -DQt6Widgets_DIR=$QT_ANDROID/lib/cmake/Qt6Widgets \
  -DQt6Network_DIR=$QT_ANDROID/lib/cmake/Qt6Network

make -j$(nproc)
make apk

echo "===== APK 构建完成 ====="
find . -name "*.apk" -exec ls -lh {} \;
