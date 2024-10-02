# Fijua-OS
## 概要
Fijua-OSはどこでも動くx64GUIマルチタスクOSである  
ブートローダーのFijua-Boot、カーネルのFijua-Kernelから構成され、GUIウインドウシステムを搭載し、Clangの生成する位置独立なELF形式のアプリケーションを起動できる。  

まずUEFIによりFijua-Bootが起動し、ELF形式のFijua-Kernelをメモリ上に展開、デバイスのほぼ全ての空いているメモリの確保、フレームバッファの取得を行いFijua-Kernelに処理を移す。
Fijua-Kernelは自身の持つモジュールの初期化を行い、ウインドウシステムの表示し、ユーザの操作に応じてアプリの起動などする。アプリケーションにはシステムコールを提供し、メモリやCPUなどのハードウェアへのアクセスを整理する  

## ビルド方法
ソースコードのルートディレクトリで`make`コマンドを実行する

## インストール方法
1. USBメモリのようなブート可能なメディアを用意  
2. FAT32にフォーマット  
3. [Githubリポジトリ](https://github.com/kntt32/Fijua-OS)のリリースからダウンロード  
4. disk以下をそのままメディアにコピー  
5. あとはメディアからブートするだけ  

## 注意点
カーネルを読み込むメモリアドレスは固定となっている。そのため、そのメモリが使用できない場合、「どこでも動く」といったもののFijua-OSは起動することができない。具体的には0x100000から0x100fff、0x200000から0x500000までが空いている必要がある。多くのパソコンではこの領域のメモリは開いているため問題はない。
### 実機
セキュアブートには対応していないため、UEFIの設定で無効にしておく必要がある  

CSMモードは無効にしておく  

**レガシーエミュレーションはあれば有効にしておく**。レガシーエミュレーションのある実験機2台ともにて、USBメモリを認識しない、起動に失敗するなどの問題が起きたため  


### QEMU
EDK2のOVMFをファームウェアとして使用する。
デフォルトではEFI_SIMPLE_POINTER_PROTOCOL（マウス入力を取得する機能）が使用できないため、マウスが使用できない。そのため、[Githubリポジトリ](https://github.com/kntt32/Fijua-OS)にあるOVMFを使うか、この機能を有効にした上でビルドすなおす必要がある。ビルドする場合は、次のような手順を踏む。  
1. EDK2を入手し、[EDK2のWiki](https://github.com/tianocore/tianocore.github.io/wiki/Getting-Started-with-EDK-II)などを参考にEDK2本体をビルドする
2. `OvmfPkg/OvmfPkgX64.fdf`に太字部分を書き加える  
> INF MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf  
> INF MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf  
> INF MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf  
> INF MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf  
> INF MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf  
> **INF MdeModulePkg/Bus/Usb/UsbMouseDxe/UsbMouseDxe.inf**  
> INF MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf  

3. `OvmfPkg/OvmfPkgX64.dsc`に太字部分を書き加える
>   MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf  
>   MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf  
>   MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf  
>   MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf  
>   MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf  
> 	**MdeModulePkg/Bus/Usb/UsbMouseDxe/UsbMouseDxe.inf**  
>   MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf  

4. OvmfPkgディレクトリに移動し、次のコマンドを実行する
> ./build.sh -a X64 -p OvmfPkgX64.dsc

成功すれば、`/Build/OvmfX64/(環境によって異なる)/FV/OVMF.fd`に生成物が保存される

QEMUはパスを通し、起動ディスクを指定した上で、以下のようなオプションを追加して起動する。$(OVMFFDPATH)にはOVMF.fdのパスを入れる。  
> --monitor stdio -usb -device usb-mouse -device usb-kbd -bios $(OVMFFDPATH)
