# Fijua-OSの使い方

## 画面の説明
### デスクトップ
起動すると、上部にタスクバーのあるデスクトップ画面が表示される。左上にはシェルとファイルマネージャーのロゴがあり、クリックするとそれらが起動する。右上の電源ロゴをクリックすると、シャットダウンするか確認するダイアログが表示される。

### シェル
シェルでは、様々なコマンドが使える。パスは相対パスで指定する。多くの操作はファイルマネージャーで可能であり、ディレクトリへの操作の利便性などから、メインでは、ファイルマネージャーの方を使うことを想定している。
- `cls`: 画面クリア
- `echo [msg]`: msgを表示する
- `ls [optional path]`: pathのディレクトリの中身を表示する。指定なしなら、作業ディレクトリの中身が表示される
- `cd [optional path]`: pathに移動する。pathが指定されていない場合、ルートディレクトリに移動する
- `mv [src] [dest]`: srcをdestに移動
- `cp [src] [dest]`: srcをdestにコピー
- `rm [path]`: pathを削除する
- `mkdir [path]`: ディレクトリ作成
- `cat [path]`: テキストファイルとして表示
- `touch [path]`: テキストファイルを作成
- `shutdown`: すぐにシャットダウンする
- `run [path] [option arg]`: pathの実行可能ファイルをargのインライン引数で実行する

### ファイルマネージャ
ウィンドウ上部に、ディレクトリやテキストファイルを作成するボタンがあり、クリックすると、作業中のディレクトリで作成したいファイルまたはディレクトリの名前が尋ねられる。その下には左から、作業ディレクトリを戻るボタン、パスバー、リロードボタンがある。パスバーには、作業中のパスが表示され、直接文字入力することもできる。さらにその下にはファイルリストがある。ファイルリストでは、項目ごとに「Del」や「Mov」「Cpy」のようにさまざまな操作ができる。「Mov」や「Cpy」をクリックすると、移動先又はコピー先を尋ねられる。項目の右の方に「>」がある場合、それはディレクトリであり、クリックすれば移動できる。「Open」であれば、メモ帳で開き、編集できる。「Run」ボタンをクリックすると、対応する実行可能ファイルなら、実行される。

### メモ帳
ファイルマネージャの「Open」ボタンか、シェルで`run app/notepad.elf [filename]`で絶対パスでファイルを指定して、テキストファイルを開くことができる。マウスで文字を選択し、右クリックで、文字列選択状態ならコピー、そうでないならペーストされる。この機能はファイルマネージャーのパスバー含む、OS上のエディタ全てで使用できる。編集したファイルを保存するには、「Save」ボタンをクリックする。

## インストール方法
1. USBメモリのようなブート可能なメディアを用意  
2. FAT32にフォーマット  
3. リリースなどから取得したdisk以下を、そのままメディアのルートディレクトリにコピー  
4. あとはメディアからブートするだけ  

## 注意点
カーネルを読み込むメモリアドレスは固定となっている。そのため、そのメモリが使用できない場合、「どこでも動く」といったもののFijua-OSは起動することができない。具体的には0x100000から0x100fff、0x200000から0x500000までが空いている必要がある。多くのパソコンではこの領域のメモリは開いているため問題はない。
### 実機
セキュアブートには対応していないため、UEFIの設定で無効にしておく必要がある  

CSMモードは無効にしておく  

**レガシーエミュレーションはあれば有効にしておく**。レガシーエミュレーションのある実験機2台ともにて、USBメモリを認識しない、起動に失敗するなどの問題が起きたため  


### QEMU
EDK2のOVMFをファームウェアとして使用する。
デフォルトではEFI_SIMPLE_POINTER_PROTOCOL（マウス入力を取得する機能）が使用できないため、マウスが使用できない。そのため、この機能を有効にした上でOVMFをビルドすなおす必要がある。次のような手順を踏む。  
1. EDK2を入手し、[EDK2のWiki](https://github.com/tianocore/tianocore.github.io/wiki/Getting-Started-with-EDK-II)などを参考にEDK2本体をビルドする
2. `OvmfPkg/OvmfPkgX64.fdf`に"" ~ ""の部分を書き加える
```
INF MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
INF MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
INF MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
INF MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
INF MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
"" INF MdeModulePkg/Bus/Usb/UsbMouseDxe/UsbMouseDxe.inf ""
INF MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf
```

3. `OvmfPkg/OvmfPkgX64.dsc`に"" ~ ""の部分を書き加える
```
MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
"" MdeModulePkg/Bus/Usb/UsbMouseDxe/UsbMouseDxe.inf ""
MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf
```

5. OvmfPkgディレクトリに移動し、次のコマンドを実行する
```
./build.sh -a X64 -p OvmfPkgX64.dsc
```

成功すれば、`/Build/OvmfX64/(環境によって異なる)/FV/OVMF.fd`に生成物が保存される

QEMUはパスを通し、起動ディスクを指定した上で、以下のようなオプションを追加して起動する。$(OVMFFDPATH)にはOVMF.fdのパスを入れる。  
```
--monitor stdio -usb -device usb-mouse -device usb-kbd -m 512M -bios $(OVMFFDPATH)
```
