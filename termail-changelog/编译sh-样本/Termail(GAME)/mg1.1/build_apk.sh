#!/bin/bash
# ═══════════════════════════════════════════
#   Termail Games 构建脚本
# ═══════════════════════════════════════════

export ANDROID_HOME=/home/sheng/Android/sdk
export ANDROID_NDK_ROOT=$ANDROID_HOME/ndk/25.1.8937393
export QT_ANDROID=/home/sheng/Qt6_android/6.5.0/android_arm64_v8a
export QT_HOST=/home/sheng/Qt6_host/6.5.0/gcc_64

GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

echo ""
echo -e "${BLUE}═══════════════════════════════════════${NC}"
echo -e "${BLUE}  Termail Games 构建脚本${NC}"
echo -e "${BLUE}═══════════════════════════════════════${NC}"
echo ""

cd /home/sheng/Termail_Games
rm -rf build_android
mkdir build_android && cd build_android

echo -e "${BLUE}[步骤]${NC} CMake 配置中..."
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-33 \
  -DANDROID_SDK_ROOT=$ANDROID_HOME \
  -DQT_HOST_PATH=$QT_HOST \
  -DCMAKE_PREFIX_PATH=$QT_ANDROID \
  -DQt6_DIR=$QT_ANDROID/lib/cmake/Qt6 \
  -DQt6Widgets_DIR=$QT_ANDROID/lib/cmake/Qt6Widgets \
  -DQt6Core_DIR=$QT_ANDROID/lib/cmake/Qt6Core \
  -DQt6Gui_DIR=$QT_ANDROID/lib/cmake/Qt6Gui
  
if [ $? -ne 0 ]; then
    echo -e "${RED}[错误]${NC} CMake 配置失败"
    exit 1
fi

echo -e "${BLUE}[步骤]${NC} 编译中..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo -e "${RED}[错误]${NC} 编译失败"
    exit 1
fi

echo -e "${BLUE}[步骤]${NC} 配置 Gradle..."
mkdir -p android-build
cat > android-build/gradle.properties << 'EOF'
org.gradle.jvmargs=-Xmx1024m -Dfile.encoding=UTF-8
org.gradle.daemon=false
android.aapt2FromMavenOverride=/home/sheng/Android/sdk/build-tools/33.0.0/aapt2
EOF

echo -e "${BLUE}[步骤]${NC} 打包 APK..."
make apk

if [ $? -ne 0 ]; then
    echo -e "${RED}[错误]${NC} APK 打包失败"
    exit 1
fi

echo ""
echo -e "${GREEN}═══════════════════════════════════════${NC}"
echo -e "${GREEN}  ✅ APK 构建完成！${NC}"
echo -e "${GREEN}═══════════════════════════════════════${NC}"
echo ""

find . -name "*.apk" -exec ls -lh {} \;
echo ""
