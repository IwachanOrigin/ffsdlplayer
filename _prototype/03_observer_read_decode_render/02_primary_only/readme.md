
# 03 observer read decode render / primary only

## Introduction

このプログラムは、Primary/Secondaryのアイデアを捨てることにしたバージョン  
フォルダ内のファイルを再生するのだが、プレイヤーはファイル再生が終わる度に  
終了・起動を繰り返す仕組みになる。  

起動と終了の判断はControllerが担う。  
通知はreader/decoder/rendererがオブザーバーパターンを利用して行う。  
