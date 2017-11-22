#! /bin/bash

PROFILE_FILE=~/.bashrc
QT_INSTALL_DIR="$HOME"

sudo apt-get install python3-pyqt5
sudo apt-get install python3-pyqt5.qsci
sudo apt-get install python3-pyqt5.qtmultimedia
sudo apt-get install python3-pyqt5.qtopengl

sudo apt-get install python3-pip
sudo pip3 install --upgrade pip
sudo pip3 install numpy
sudo pip3 install pyserial
sudo pip3 install pyopengl

echo export LD_LIBRARY_PATH="$QT_INSTALL_DIR/Qt/5.9.2/qcc_64/lib/":'$LD_LIBRARY_PATH' >> "$PROFILE_FILE"
