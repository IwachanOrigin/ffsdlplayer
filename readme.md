
# ffsdlplayer

## Introduction

    This video player use to ffmpeg(v6.0+) and SDL2.  

## Dependency

    - SDL2 2.28.1  
    - FFMPEG 6.0+  

## Setup

### For Windows

#### SDL2

SDL2は公式サイトよりダウンロードし、任意のディレクトリへ配置してください。  
[SDL Homepage](https://www.libsdl.org/)  

#### FFMPEG

FFMPEGは公式サイトよりダウンロードし、任意のディレクトリへ配置してください。  
[FFMPEG Download](https://ffmpeg.org/download.html)  

NOTE1: LGPLバージョンをダウンロードしてください。  
NOTE2: ビルドにはversion 6.0を利用しています。version 6.1以上の場合、DLL名など変更する必要があります。  

### For Ubuntu

#### SDL2

    sudo apt install libsdl2-dev  
    sudo apt install libsdl2-image-dev libsdl2-mixer-dev libsdl2-net-dev libsdl2-ttf-dev  

#### FFMPEG

NOTE1: [Ubuntu22.04へFFMPEG6をインストール](https://ubuntuhandbook.org/index.php/2023/03/ffmpeg-6-0-released-how-to-install-in-ubuntu-22-04-20-04/)  
NOTE2: Savoury1 のPPAは依存関係の問題を解決できず、インストールに失敗します。  

FFMPEGは6.0以降をインストールする必要があります。  
以下の手順でPPAよりインストールを行います。  

1. ffmpeg6.0のPPAを追加します。  

``` shell
sudo add-apt-repository ppa:ubuntuhandbook1/ffmpeg6
```

2. update を行います。  

``` shell
sudo apt update  
```

3. ffmpeg 6.0をインストールします  

``` shell
sudo apt install ffmpeg
```

4. インストール可能なlibav...を確認します。  

``` shell
apt policy libavcodec-dev libavfilter-dev libavformat-dev libavutil-dev libavdevice-dev libswscale-dev libswresample-dev  
```

以下のような出力が得られると思います。  
[Installed:]はインストール済みのものです。  

``` text
-------------------------------------------------------------------------------------------------------------
libavcodec-dev:
  Installed: 7:6.0-1build8~22.04
  Candidate: 7:6.0-1build8~22.04
  Version table:
 *** 7:6.0-1build8~22.04 500
        500 https://ppa.launchpadcontent.net/ubuntuhandbook1/ffmpeg6/ubuntu jammy/main amd64 Packages
        100 /var/lib/dpkg/status
     7:6.0-0ubuntu1~22.04.sav2 500
        500 https://ppa.launchpadcontent.net/savoury1/ffmpeg6/ubuntu jammy/main amd64 Packages
     7:4.4.4-0ubuntu1~22.04.sav1 500
        500 https://ppa.launchpadcontent.net/savoury1/ffmpeg4/ubuntu jammy/main amd64 Packages
     7:4.4.2-0ubuntu0.22.04.1 500
        500 http://archive.ubuntu.com/ubuntu jammy-updates/universe amd64 Packages
        500 http://security.ubuntu.com/ubuntu jammy-security/universe amd64 Packages
     7:4.4.1-3ubuntu5 500
        500 http://archive.ubuntu.com/ubuntu jammy/universe amd64 Packages
-------------------------------------------------------------------------------------------------------------
```


5. 必要なバージョンをインストールします。  
   指定は3つだけですが、他のlibav...は依存しているので勝手にインストールされます。  

``` shell
sudo apt install libavformat-dev=7:6.0-1build8~22.04 libavfilter-dev=7:6.0-1build8~22.04 libavdevice-dev=7:6.0-1build8~22.04  
```

6. 全てインストールされたかを確認します。  

``` shell
apt policy libavcodec-dev libavfilter-dev libavformat-dev libavutil-dev libavdevice-dev libswscale-dev libswresample-dev  
```


## Build

FFMPEGはversion 6.0を利用しています。version 6.1以上の場合、DLL名など変更する必要があります。  

### For Windows

#### Debug

- Ninja + LLVM 16.0  

    powershell.exe cmake -S . -B build -G "\"Ninja Multi-Config"\" -D FFMPEG_PATH="/path/to/ffmpeg" -D SDL2_PATH="/path/to/sdl2"  

``` shell
powershell.exe cmake -S . -B build -G "\"Ninja Multi-Config\"" -D FFMPEG_PATH="C:\software\ffmpeg-n6.0-latest-win64-lgpl-shared-6.0" -D SDL2_PATH="C:\software\SDL2-2.28.1"  
powershell.exe cmake --build build  
```

### For Ubuntu(22.04 LTS)

#### Debug

- GCC 11.4.0(Ubuntu 11.4.0-1ubuntu1~22.04)

``` shell
cmake -S . -B build  
cmake --build build  
```


