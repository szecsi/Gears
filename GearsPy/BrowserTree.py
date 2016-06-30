import sys
import Gears as gears

from PyQt5.QtCore import (Qt, QCoreApplication, QTimer, QSize)
from PyQt5.QtWidgets import (QWidget, QMessageBox, QApplication, QTreeWidget, QTreeWidgetItem, QTreeWidgetItemIterator, QGridLayout, QLabel, QSpacerItem, QSizePolicy)
from PyQt5.QtGui import (QFont, QPalette )
from SequenceLoader import *

class BrowserTree(QTreeWidget):
    rootPath = "./Project/Sequences"
    launcherWindow = None
    browserWindow = None

    def __init__(self, launcherWindow, browserWindow):
        super().__init__()
        self.launcherWindow = launcherWindow
        self.browserWindow = browserWindow
        self.itemClicked.connect(self.onClick)
        self.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed )
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.itemCollapsed.connect(self.onCollapse)
        self.itemExpanded.connect(self.onExpand)

    def open(self, item):
        if(item.text(0).endswith('.pyx')):
            fullpath = ''
            p = item
            while p:
                fullpath = '/' + p.text(0) + fullpath
                p = p.parent()
            fullpath = self.rootPath + fullpath

            if loadSequence(fullpath, self):
                self.launcherWindow.start(self.browserWindow, fullpath)
                QApplication.instance().processEvents()
                self.browserWindow.hide()
        else:
            if item.isExpanded() :
                self.collapseItem(item)
            else :
                self.expandItem(item)

    def onExpand(self, item):
        self.resizeColumnToContents(0)
        self.updateGeometry()

    def onCollapse(self, item):
        self.resizeColumnToContents(0)
        self.updateGeometry()

    def onClick(self, item, int):
        self.open(item)

#    def sizeHintForColumn(self, iCol):
#        width = 0
#        itemit = QTreeWidgetItemIterator(self, QTreeWidgetItemIterator.NotHidden)
#        while itemit.value() :
#            rect = self.visualItemRect( itemit.value() )
#            width = max(width, rect.right())
#            itemit += 1
#        return width

    def sizeHint(self):
        height = self.header().height() + 2
        itemit = QTreeWidgetItemIterator(self, QTreeWidgetItemIterator.NotHidden)
        while itemit.value() :
            rect = self.visualItemRect( itemit.value() )
            height += rect.height()
            itemit += 1
        width = self.columnWidth(0) 
        scrollb = self.scrollBarWidgets(Qt.AlignRight)
        if scrollb :
            width += scrollb[0].size().width()
        return QSize(width, height) 
