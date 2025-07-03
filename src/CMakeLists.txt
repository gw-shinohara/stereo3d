cmake_minimum_required(VERSION 3.16)
project(QtPointCloudViewer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ビルド時に自動でMOC, UIC, RCCを実行する
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Qt6の必要なモジュールを探す
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)

# 実行ファイルを作成 (ソースファイルのパスを修正)
add_executable(my_app main.cpp)

# happlyライブラリのヘッダファイルへのパスを追加
target_include_directories(my_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/happly)

# ライブラリをリンク
target_link_libraries(my_app PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
)
