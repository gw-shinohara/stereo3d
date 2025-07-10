# ベースイメージとしてUbuntu 22.04 を使用
FROM ubuntu:22.04

# 環境変数を設定し、apt実行時の対話を抑制
ENV DEBIAN_FRONTEND=noninteractive

# 必要なパッケージのインストール
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    cmake \
    qt6-base-dev \
    libqt6opengl6-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libxkbcommon-dev \
    libvulkan-dev \
    fonts-noto-cjk \
    --no-install-recommends && \
    rm -rf /var/lib/apt/lists/*

# 作業ディレクトリを作成
WORKDIR /app
