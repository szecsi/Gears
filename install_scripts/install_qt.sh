#! /bin/bash

QT_INSTALL_DIR=~/
cd "$QT_INSTALL_DIR"

wget http://download.qt.io/official_releases/online_installers/qt-unified-linux-x64-online.run
chmod +x qt-unified-linux-x64-online.run
./qt-unified-linux-x64-online.run
rm qt-unified-linux-x64-online.run

