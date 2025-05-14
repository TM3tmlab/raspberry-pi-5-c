# raspberry-pi-5-c

Raspberry Pi 5 を使った C のサンプルプログラム

## 環境

- Raspberry Pi 5 4GB
- Raspberry Pi OS Bookworm
- libgpiod 1.6.2

## セットアップ

導入済み

- build-essential(gcc, make, etc...)
- libgpiod

### apt

```shell
sudo apt update
sudo apt install libgpiod-dev
```

### clone

```shell
git clone https://github.com/TM3tmlab/raspberry-pi-5-c.git
```

### build

```shell
make
```

## see

このプログラムは Unlicense ライセンスのもとで公開されています（詳細は LICENSE を参照してください）。

本プログラムは `libgpiod` ライブラリを使用しており、このライブラリは GNU Lesser General Public License（LGPL）のもとでライセンスされています。  
コンパイル済みバイナリを配布する際は、libgpiod のライセンス条件を遵守してください。

This program is released under the Unlicense (see LICENSE).

It uses the `libgpiod` library, which is licensed under the GNU Lesser General Public License (LGPL).
Ensure that you comply with libgpiod's license terms when distributing compiled binaries.

## link

- [libgpiod c if](https://libgpiod.readthedocs.io/en/stable/index.html)
