# raspberry-pi-5-c

Raspberry Pi 5 を使った C のサンプルプログラム集

## 環境

- Raspberry Pi 5 4GB
- Raspberry Pi OS Bookworm
- libpgpio 1.6.2

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

## link

- [libgpiod c if](https://libgpiod.readthedocs.io/en/stable/index.html)
