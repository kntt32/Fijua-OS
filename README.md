# Fijua-OS
## 概要
Fijua-OSはどこでも動くx64GUIマルチタスクOSである  
ブートローダーのFijua-Boot、カーネルのFijua-Kernelから構成され、GUIウインドウシステムを搭載し、Clangの生成する位置独立なELF形式のアプリケーションを起動できる。  

まずUEFIによりFijua-Bootが起動し、ELF形式のFijua-Kernelをメモリ上に展開、デバイスのほぼ全ての空いているメモリの確保、フレームバッファの取得を行いFijua-Kernelに処理を移す。
Fijua-Kernelは自身の持つモジュールの初期化を行い、ウインドウシステムの表示し、ユーザの操作に応じてアプリの起動などする。アプリケーションにはシステムコールを提供し、メモリやCPUなどのハードウェアへのアクセスを整理する  

## ビルド方法
ソースコードのルートディレクトリで`make`コマンドを実行する

## インストール方法
[HOWTOUSE.md](HOWTOUSE.md)を参照

## これからやりたいこと
### 短期的な目標
- ブートローダーおよびカーネルの安全な言語への段階的な置き換えによる安定性の向上
    - Rustを使おうと考えていたが、コンパイル速度やセルフホストをすることを考え、自作中の言語というか、安全で高速で効率的にコーディングできる（予定の）多機能アセンブラ[AsMacro](https://github.com/kntt32/AsMacro-lang)に置き換えることを検討中
- 細かなモジュール化
- テストアプリを増やす
- Rustアプリケーション用のリロケーションへの対応
- Fijua-OSアプリケーション用Rustの、縮小版allocおよびstdクレートの作成
- 自動キャッシュによるストレージアクセスの高速化
- テキストベースの自作ブラウザの搭載
### 長期目標
- Linuxアプリケーションへの対応
- x64以外のプラットフォームへの対応（Arm, Riscvなど）
- セルフホスト
