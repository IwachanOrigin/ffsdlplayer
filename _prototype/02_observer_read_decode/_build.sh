#!/bin/bash

# Defined help message
show_help()
{
    echo "Usage: _build.sh opt1 opt2 ... optN"
    echo "Options"
    echo " opt01           1 is Ninja build. 0 is default."
    echo " opt02           1 is Clean build."
    echo " opt03           1 is Release build. 0 is Debug."
    echo " opt04           1 is Run Test."
    echo " -h, --help      Display this help message"
}

# If arguments are empty or first argument is h/-h/--help, call show_help func.
if [[ -z "$1" || "$1" == "h" || "$1" == "-h" || "$1" == "--help" ]]; then
    show_help
    exit 0
fi

# Received argument
# 1 is build to ninja multi-config
# other is build to vsbuild
BUILD_FLAG=$1

# 1 is clean
CLEAN_FLAG=$2

# 1 is Release build
RELEASE_FLG=$3

# 1 is Run test
TEST_RUN=$4

# Set Qt_install_dir
#QT="C:/software/Qt/6.5.3/msvc2019_64"
FFMPEG_PATH="C:/software/ffmpeg-n7.0-latest-win64-lgpl-shared-7.0"
SDL2_PATH="C:/software/SDL2-2.30.7"

echo "FFMPEG_PATH = ${FFMPEG_PATH}"
echo "SDL2_PATH = ${SDL2_PATH}"

# Set Build_dir
BUILD_DIR="build"

if [ "$CLEAN_FLAG" == "1" ]; then
    rm -rf $BUILD_DIR
fi

OPT_BUILD_MODE="debug"
if [ "$RELEASE_FLG" == "1" ]; then
    OPT_BUILD_MODE="release"
fi

ENABLE_QT_GUI_FLG="OFF"
if [ "$USE_QT_GUI" == "1" ]; then
    ENABLE_QT_GUI_FLG="ON"
fi

if [ "$BUILD_FLAG" == "1" ]; then
    powershell.exe cmake -S . -B build -D FFMPEG_PATH=${FFMPEG_PATH} -D SDL2_PATH=${SDL2_PATH} -G "\"Ninja Multi-Config\""
    cp $BUILD_DIR/compile_commands.json ./compile_commands.json
    powershell.exe cmake --build $BUILD_DIR --config=$OPT_BUILD_MODE -- -j $(nproc) 
else
    powershell.exe cmake -S . -B build -D FFMPEG_PATH=${FFMPEG_PATH} -D SDL2_PATH=${SDL2_PATH} -D config=$OPT_RELEASE
    powershell.exe cmake --build $BUILD_DIR --config=$OPT_BUILD_MODE
fi

if [ "$TEST_RUN" == "1" ]; then
    cd $BUILD_DIR
    powershell.exe ctest
fi


