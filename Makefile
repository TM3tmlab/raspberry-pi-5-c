# コンパイラ
CC = gcc

# ライブラリ
LIBS = -lgpiod
FLAGS = -Wall -Wextra -Werror

# ディレクトリ
SRC_DIR = src
BIN_DIR = bin

# 全Cソースファイル
SRC = $(wildcard $(SRC_DIR)/*.c)

# 出力ファイル名（src/foo.c → bin/foo）
TARGETS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%, $(SRC))

# デフォルトルール
all: $(BIN_DIR) $(TARGETS)

# binディレクトリが存在しない場合に作成
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# 各ターゲットビルドルール
$(BIN_DIR)/%: $(SRC_DIR)/%.c
	$(CC) -o $@ $< $(LIBS) $(FLAGS)

# クリーンアップルール
clean:
	rm -f $(BIN_DIR)/*
