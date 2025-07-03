# C++開発環境を管理するためのMakefile

# .PHONY はファイル名と競合しないようにターゲットを定義します
.PHONY: all run build shell clean setup download-data check-submodule check-data xhost-allow xhost-deny help

# --- 変数定義 ---
# サンプルデータ
PLY_URL := https://people.sc.fsu.edu/~jburkardt/data/ply/cube.ply
PNG_URL := https://raw.githubusercontent.com/huggingface/datasets/main/test.png
DATA_DIR := data
PLY_FILE := $(DATA_DIR)/sample.ply
PNG_FILE := $(DATA_DIR)/sample.png

# Git Submodule
SUBMODULE_PATH := src/happly
SUBMODULE_URL := https://github.com/nmwsharp/happly.git

# --- メインターゲット ---

# デフォルトターゲット: `make` コマンドで `make run` が実行されます
all: run

# アプリケーションのビルドと実行
run: check-submodule check-data xhost-allow
	@echo "--- アプリケーションコンテナをビルド・実行します... ---"
	docker compose up --build

# Dockerイメージをビルドします (コンテナは実行しません)
build:
	@echo "--- Dockerイメージをビルドします... ---"
	docker compose build

# デバッグや手動操作のためにコンテナのシェルに入ります
shell: check-submodule check-data xhost-allow
	@echo "--- コンテナのシェルを起動します... ---"
	docker compose run --rm app bash

# コンテナ、ネットワーク、ボリュームを停止・削除します
clean:
	@echo "--- Dockerコンテナとネットワークをクリーンアップします... ---"
	docker compose down --volumes --remove-orphans

# --- セットアップとチェック ---

# 前提条件をセットアップします (git submoduleとサンプルデータ)
setup:
	@echo "--- 前提条件をセットアップします... ---"
	@echo "  - git submoduleのセットアップ (happly)..."
	@if ! git config --file .gitmodules --get-regexp path | grep -q $(SUBMODULE_PATH); then \
		echo "    - submodule 'happly' をプロジェクトに追加します..."; \
		git submodule add $(SUBMODULE_URL) $(SUBMODULE_PATH); \
	else \
		echo "    - submodule 'happly' は既に設定済みです。"; \
	fi
	@echo "  - git submoduleの初期化/更新..."
	@git submodule update --init --recursive
	@echo "  - サンプルデータのダウンロード..."
	@$(MAKE) download-data

# サンプルデータをダウンロードします
download-data:
	@mkdir -p $(DATA_DIR)
	@echo "    - $(PLY_FILE) をダウンロード中..."
	@curl -s -L $(PLY_URL) -o $(PLY_FILE)
	@echo "    - $(PNG_FILE) をダウンロード中..."
	@curl -s -L $(PNG_URL) -o $(PNG_FILE)

# git submoduleが初期化されているか確認します
check-submodule:
	@if [ ! -f "$(SUBMODULE_PATH)/happly.h" ]; then \
		echo "!!! [エラー] git submoduleが初期化されていません。'make setup' を実行してください。"; \
		exit 1; \
	fi

# サンプルデータが存在するか確認します
check-data:
	@if [ ! -f "$(PLY_FILE)" ] || [ ! -f "$(PNG_FILE)" ]; then \
		echo "!!! [エラー] サンプルデータが見つかりません。'make setup' を実行してください。"; \
		exit 1; \
	fi

# --- X11関連 ---

# GUI転送のためにXサーバーへのローカル接続を許可します
xhost-allow:
	@echo "--- X11のローカル接続を許可します... ---"
	@xhost +local:docker > /dev/null

# Xサーバーへのローカル接続許可を取り消します
xhost-deny:
	@echo "--- X11のローカル接続許可を取り消します... ---"
	@xhost -local:docker > /dev/null

# --- ヘルプ ---

# ヘルプメッセージを表示します
help:
	@echo "使用法: make [ターゲット]"
	@echo ""
	@echo "ターゲット一覧:"
	@echo "  run          (デフォルト) アプリケーションをビルドして実行します。"
	@echo "  build        Dockerイメージをビルドします。"
	@echo "  shell        コンテナのインタラクティブシェルを起動します。"
	@echo "  setup        前提条件（git submoduleとサンプルデータ）をセットアップします。"
	@echo "  clean        関連するすべてのコンテナとネットワークを停止・削除します。"
	@echo "  help         このヘルプメッセージを表示します。"

