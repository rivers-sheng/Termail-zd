#!/bin/bash
# ═══════════════════════════════════════════
#   Termail v6.1- Android APK 构建脚本
# ═══════════════════════════════════════════

# ---------- 环境变量 ----------
export ANDROID_HOME=/home/sheng/Android/sdk
export ANDROID_NDK_ROOT=$ANDROID_HOME/ndk/25.1.8937393
export QT_ANDROID=/home/sheng/Qt6_android/6.5.0/android_arm64_v8a
export QT_HOST=/home/sheng/Qt6_host/6.5.0/gcc_64

# ---------- 颜色 ----------
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# ---------- 函数 ----------
print_step() {
    echo -e "${BLUE}[步骤]${NC} $1"
}

print_ok() {
    echo -e "${GREEN}[成功]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[警告]${NC} $1"
}

print_error() {
    echo -e "${RED}[错误]${NC} $1"
}

# ---------- 开始 ----------
echo ""
echo -e "${BLUE}═══════════════════════════════════════${NC}"
echo -e "${BLUE}  Termail v6.1 构建脚本${NC}"
echo -e "${BLUE}═══════════════════════════════════════${NC}"
echo ""

# ---------- 清理旧构建 ----------
print_step "清理旧构建目录..."
rm -rf /home/sheng/Termail_v6.1/build_android
mkdir -p /home/sheng/Termail_v6.1/build_android
cd /home/sheng/Termail_v6.1/build_android
print_ok "清理完成"

# ---------- CMake 配置 ----------
print_step "CMake 配置中..."
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
  -DQt6Gui_DIR=$QT_ANDROID/lib/cmake/Qt6Gui \
  -DQt6Network_DIR=$QT_ANDROID/lib/cmake/Qt6Network

if [ $? -ne 0 ]; then
    print_error "CMake 配置失败"
    exit 1
fi
print_ok "CMake 配置完成"

# ---------- 编译 ----------
print_step "编译中..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    print_error "编译失败"
    exit 1
fi
print_ok "编译完成"

# ---------- Gradle 配置 ----------
print_step "配置 Gradle..."
mkdir -p android-build
cat > android-build/gradle.properties << 'EOF'
org.gradle.jvmargs=-Xmx1024m -Dfile.encoding=UTF-8
org.gradle.daemon=false
android.aapt2FromMavenOverride=/home/sheng/Android/sdk/build-tools/33.0.0/aapt2
EOF
print_ok "Gradle 配置完成"

# ---------- 打包 APK ----------
print_step "打包 APK..."
make apk

if [ $? -ne 0 ]; then
    print_error "APK 打包失败"
    exit 1
fi

# ---------- 完成 ----------
echo ""
echo -e "${GREEN}═══════════════════════════════════════${NC}"
echo -e "${GREEN}  ✅ APK 构建完成！${NC}"
echo -e "${GREEN}═══════════════════════════════════════${NC}"
echo ""

# 显示 APK 信息
find . -name "*.apk" -exec ls -lh {} \;

echo ""
print_ok "构建脚本执行完毕"
echo ""
