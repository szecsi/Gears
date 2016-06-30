import sys
import Gears as gears
import importlib.machinery
import os
import re
from PyQt5.QtCore import (Qt, QCoreApplication, QTimer, QAbstractItemModel, QEvent )
from PyQt5.QtWidgets import (QWidget, QToolTip, QPushButton, QMessageBox, QApplication, QTreeWidget, QTreeWidgetItem, QGridLayout, QLabel, QSpacerItem, QSizePolicy, QStackedLayout)
from PyQt5.QtGui import (QFont, QPalette, QPixmap, QIcon)

from BrowserTree import BrowserTree

def empty_dirs(root_dir='.', recursive=True):
    empty_dirs = []
    for root, dirs, files in os.walk(root_dir, topdown=False):
        #print root, dirs, files
        if recursive:
            all_subs_empty = True  # until proven otherwise
            for sub in dirs:
                full_sub = os.path.join(root, sub)
                if full_sub not in empty_dirs:
                    #print(full_sub, "not empty")
                    all_subs_empty = False
                    break
        else:
            all_subs_empty = (len(dirs) == 0)
        if all_subs_empty and is_empty(files):
            empty_dirs.append(root)
            yield root


def is_empty(files):
    empty = True
    for f in files:
        if f.endswith('.pyx'):
            empty = False
    return empty

def find_empty_dirs(root_dir='.', recursive=True):
    return list(empty_dirs(root_dir, recursive))


class BrowserWindow(QWidget):
    launcherWindow = None
    logoPixmap = None

    def __init__(self, launcherWindow, app):
        super().__init__()
        self.app = app
        self.launcherWindow = launcherWindow
        self.fontSize = 14
        self.fontSizeLarge = 24
        self.fontSizeSmall = 10
        self.initUI()
        
    def multiple_replace(self, string, rep_dict):
        pattern = re.compile("|".join([re.escape(k) for k in rep_dict.keys()]), re.M)
        return pattern.sub(lambda x: rep_dict[x.group(0)], string)

    def styleEsc(self, string):
        return self.multiple_replace(string, {'{' : '{{', '}' : '}}', '@<' : '{', '>@' : '}' })

    def initUI(self):
##        pal = QPalette();
##        pal.setColor(QPalette.Background, Qt.black);
##        self.setPalette(pal);
        self.setSizePolicy(QSizePolicy.Maximum, QSizePolicy.Maximum)
        self.styleTemplate = self.styleEsc("""
            QWidget{
                background-color: black;
                border-color: red;
                border-style: solid;
                border-width: 1px;
                border-radius: 0px;
                font: bold @<fontSize>@px;
                color: red;
                padding: 0px;
                font-family: "Candara";
                font-size: @<fontSize:d>@px;
            }
            QLabel{
                font-size: @<fontSizeLarge:d>@px;
                 border-width: 0px;
            }
            QTreeView::item:hover {
                background: black;
                border: 1px solid red;
            }
            QTreeView::item:selected {
                border: 1px solid red;
                color: red;
            }
            QTreeView::item:selected:active{
                background: black;
                border: 1px solid red;
            }
            QTreeView::item:selected:!active {
                border: 1px solid red;
                color: red;
            }
            QTreeView::branch:has-siblings:!adjoins-item {
                border-image: url(Gui/stylesheet-vline.png) 0;
            }

            QTreeView::branch:has-siblings:adjoins-item {
                border-image: url(Gui/stylesheet-branch-more.png) 0;
            }

            QTreeView::branch:!has-children:!has-siblings:adjoins-item {
                border-image: url(Gui/stylesheet-branch-end.png) 0;
            }

            QTreeView::branch:has-children:!has-siblings:closed,
            QTreeView::branch:closed:has-children:has-siblings {
                    border-image: none;
                    image: url(Gui/stylesheet-branch-closed.png);
            }

            QTreeView::branch:open:has-children:!has-siblings,
            QTreeView::branch:open:has-children:has-siblings  {
                    border-image: none;
                    image: url(Gui/stylesheet-branch-open.png);
            }

            QScrollBar:vertical {
                 border: 2px solid red;
                 background: black;
                 width: 40px;
                 margin: 22px 0 22px 0;
             }
             QScrollBar::handle:vertical {
                 background: red;
                 min-height: 20px;
             }
             QScrollBar::add-line:vertical {
                 border: 2px red;
                 background: black;
                 height: 20px;
                 subcontrol-position: bottom;
                 subcontrol-origin: margin;
             }
             QScrollBar::sub-line:vertical {
                 border: 2px red;
                 background: black;
                 height: 20px;
                 subcontrol-position: top;
                 subcontrol-origin: margin;
             }
             QScrollBar::up-arrow:vertical, QScrollBar::down-arrow:vertical {
                 border: 2px solid black;
                 width: 3px;
                 height: 3px;
                 background: red;
             }

             QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
                 background: none;
             }
            """)

        self.setStyleSheet(self.styleTemplate.format(fontSize=int(self.fontSize), fontSizeLarge=int(self.fontSizeLarge)))
        #gridWidget = QWidget(self)
        #self.gridWidget = gridWidget
        grid = QGridLayout()
        #treeMooker = QWidget(self)
        #treeMooker.setStyleSheet("background-color: transparent;")
        #treeMooker.setAttribute(Qt.WA_TransparentForMouseEvents)
        #treeMooker.installEventFilter(self)
        #treeMookerGrid = QGridLayout()
        #stack = QStackedLayout()
        #stack.setStackingMode(QStackedLayout.StackAll)

        logo = QLabel(self)
        self.logoPixmap = QPixmap("./Project/Media/Gears.png")
        logo.setPixmap( self.logoPixmap )
        logo.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        grid.addWidget( logo, 2, 3, 1, 1 )
        self.titleTemplate = self.styleEsc('''
<p align="left"><strong>
<span style='font-size:@<fontSizeLarge>@pt; font-weight:600; color:#ff0000;'>GPU Eye And Retina Stimulation  </span></strong><span style='font-size:10pt; font-weight:600; color:#aa0000;'>beta test version 0.4</span></p>
<table border="0" cellpadding="0" cellspacing="0">
	<tbody>
		<tr>
			<td >
			<p align="left"><span style='font-size:@<fontSize>@pt; font-weight:600; color:#ff0000;'>László Szécsi</span></p>
			</td>
			<td >
			<p><span style='font-size:@<fontSizeSmall>@pt; font-weight:600; color:#aa0000;'>Budapest University of Technology and Economics</span></p>
			</td>
		</tr>
		<tr>
			<td >
			<p align="left"> <span style='font-size:@<fontSize>@pt; font-weight:600; color:#ff0000;'>Péter Hantz</span></p>
			</td>
			<td >
			<p><span style='font-size:@<fontSizeSmall>@pt; font-weight:600; color:#aa0000;'>Ecole Polytechnique Federale de Lausanne</span></p>
			</td>
		</tr>
		<tr>
			<td >
			<p align="left"> <span style='font-size:@<fontSize>@pt; font-weight:600; color:#ff0000;'>Ágota Kacsó</span></p>
			</td>
			<td >
			<p><span style='font-size:@<fontSizeSmall>@pt; font-weight:600; color:#aa0000;'>Budapest University of Technology and Economics</span></p>
			</td>
		</tr>
		<tr>
			<td >
			<p align="left"><span style='font-size:@<fontSize>@pt; font-weight:600; color:#ff0000;'>Günther Zeck</span></p>
			</td>
			<td >
			<p><span style='font-size:@<fontSizeSmall>@pt; font-weight:600; color:#aa0000;'>Natural and Medical Sciences Institute Reutlingen</span></p>
			</td>
		</tr>
		<tr>
			<td >
			<p align="left"><span style='font-size:@<fontSizeSmall>@pt; font-weight:600; color:#aa0000;'>http://www.gears.vision</span></p>
			</td>
			<td >
            <p><span style='font-size:@<fontSizeSmall>@pt; font-weight:600; color:#aa0000;'>support@gears.vision</span></p>
			</td>
		</tr>
	</tbody>
</table>
                ''' )
        self.titleLabel = QLabel(self.titleTemplate.format(fontSize=self.fontSize, fontSizeLarge=self.fontSizeLarge, fontSizeSmall=self.fontSizeSmall))
        self.titleLabel.setTextFormat( Qt.RichText )
        self.titleLabel.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        grid.addWidget( self.titleLabel, 1, 3, 1, 1 )

        specs = QLabel(gears.getSpecs())
        #self.titleLabel.setTextFormat( Qt.RichText )
        self.titleLabel.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        specs.setFocusPolicy(Qt.NoFocus)
        #specsButton.clicked.connect(self.onSpecs)
        grid.addWidget(specs, 4, 1, 1, 1)
        
        quitButton = QPushButton('Quit')
        quitButton.setFocusPolicy(Qt.NoFocus)
        quitButton.clicked.connect(self.onQuit)
        grid.addWidget(quitButton, 4, 3, 1, 1)

        tree = BrowserTree(self.launcherWindow, self)
        tree.setAttribute(Qt.WA_TransparentForMouseEvents, on=False)
        self.tree = tree
        tree.setFocusPolicy(Qt.NoFocus)
        tree.setHeaderHidden(True)
        tree.setStyleSheet("background-color: black;");
        item0 = QTreeWidgetItem(tree)
        item0.setText(0, 'Press space to collapse all')

        itemMap = dict()
        rootPath = "./Project/Sequences"
        emptyDirs = find_empty_dirs(rootPath)
        for (path, dirs, files) in os.walk(rootPath):
            for subdir in dirs:
                fullPath = os.path.join(path, subdir)
                if fullPath not in emptyDirs:
                    if(rootPath == path):
                        item0 = QTreeWidgetItem(tree)
                        itemMap[fullPath] = item0
                        item0.setText(0, subdir)
                    else:
                        item1 = QTreeWidgetItem()
                        itemMap[fullPath] = item1
                        item1.setText(0, subdir)
                        itemMap[path].addChild(item1)
            for item in files:
                if item.endswith('.pyx'):
                    if(rootPath == path):
                        item0 = QTreeWidgetItem(tree)
                        itemMap[os.path.join(path, item)] = item0
                        item0.setText(0, item)
                    else:
                        item1 = QTreeWidgetItem()
                        itemMap[os.path.join(path, item)] = item1
                        item1.setText(0, item)
                        itemMap[path].addChild(item1)

        #treeMookerGrid
        grid.addWidget(tree, 1, 1, 4, 3, Qt.AlignLeft | Qt.AlignTop)
        
        #qbtn = QPushButton('Quit', self)
        #qbtn.clicked.connect(QCoreApplication.instance().quit)
        #qbtn.resize(qbtn.sizeHint())
        #grid.addWidget(qbtn, 8, 9)

        #treeMooker.setLayout(treeMookerGrid)
        #gridWidget.setLayout(grid)
        #stack.addWidget(treeMooker)
        #stack.addWidget(gridWidget)
        self.setLayout(grid)

        self.setWindowTitle('O\u0398\u03a6O')
#        self.setWindowIcon(QIcon('web.png'))        
        self.showFullScreen()
        self.setFixedSize(self.size())
        self.tree.setCurrentIndex( self.tree.model().index(0, 0))

    def keyPressEvent(self, e):
        if e.key() == Qt.Key_Escape or e.text() == 'q':
            box = QMessageBox(self)
            box.setText('Are you sure you want to quit?')
            box.setWindowTitle('Please confirm!')
            box.setWindowFlags(Qt.FramelessWindowHint | Qt.Dialog);
            box.setStandardButtons(QMessageBox.Yes | QMessageBox.No )
            box.setDefaultButton(QMessageBox.No)
            if box.exec() == QMessageBox.Yes:
                self.close()
        elif e.key()==Qt.Key_0 or e.key()==Qt.Key_1 or e.key()==Qt.Key_2 or e.key()==Qt.Key_3 or e.key()==Qt.Key_4 or e.key()==Qt.Key_5 or e.key()==Qt.Key_6 or e.key()==Qt.Key_7 or e.key()==Qt.Key_8 or e.key()==Qt.Key_9 :
            self.tree.clearSelection()
            self.tree.keyboardSearch(e.text())
            sel = self.tree.selectedItems()
            if len(sel) > 0 :
                self.tree.onClick( sel[0], 0)
        elif e.key() == Qt.Key_Space or e.key() == Qt.Key_Backspace:
            self.tree.collapseAll()
            self.tree.update()
        elif e.key() == Qt.Key_Down :
            ni = self.tree.itemBelow(self.tree.currentItem())
            if(ni != None) :
                self.tree.setCurrentItem( ni )
        elif e.key() == Qt.Key_Up:
            ni = self.tree.itemAbove(self.tree.currentItem())
            if(ni != None) :
                self.tree.setCurrentItem( ni )
        elif e.key() == Qt.Key_Left:
            ni = self.tree.currentItem().parent()
            if(ni != None) :
                self.tree.setCurrentItem( ni )
        elif e.key() == Qt.Key_Right or e.key() == Qt.Key_Return or e.key() == Qt.Key_Enter:
            ni = self.tree.currentItem().child(0)
            if(ni != None) :
                self.tree.setCurrentItem( ni )
            else:
              self.tree.open(self.tree.currentItem())


    def onSpecs(self):
        box = QMessageBox(self)
        horizontalSpacer = QSpacerItem(1000, 0, QSizePolicy.Minimum, QSizePolicy.Expanding)
        box.setText( gears.getSpecs() )
        box.setWindowTitle('System specs')
        box.setWindowFlags(Qt.FramelessWindowHint | Qt.Dialog);
        box.setStandardButtons(QMessageBox.Ok)
        box.setDefaultButton(QMessageBox.Ok)
        layout = box.layout()
        layout.addItem(horizontalSpacer, layout.rowCount(), 0, 1, layout.columnCount())
        box.exec()

    def onQuit(self):
        box = QMessageBox(self)
        box.setText('Are you sure you want to quit?')
        box.setWindowTitle('Please confirm!')
        box.setWindowFlags(Qt.FramelessWindowHint | Qt.Dialog);
        box.setStandardButtons(QMessageBox.Yes | QMessageBox.No )
        box.setDefaultButton(QMessageBox.No)
        if box.exec() == QMessageBox.Yes:
            self.close()

    def wheelEvent(self, event):
        if self.app.keyboardModifiers() & Qt.ControlModifier :
            self.fontSize += event.angleDelta().y() / 120
            self.fontSizeLarge += event.angleDelta().y() / 120
            self.fontSizeSmall += event.angleDelta().y() / 120
            self.setStyleSheet(self.styleTemplate.format(fontSize=int(self.fontSize), fontSizeLarge=int(self.fontSizeLarge)))
            self.titleLabel.setText(self.titleTemplate.format(fontSize=int(self.fontSize), fontSizeLarge=int(self.fontSizeLarge), fontSizeSmall=int(self.fontSizeSmall)))
            self.tree.resizeColumnToContents(0)
            self.updateGeometry()

    #def eventFilter(self, obj, event):
    #    #if event.type() in [ QEvent.MouseMove, QEvent.MouseButtonPress, QEvent.MouseButtonRelease, QEvent.MouseButtonDblClick]:
    #    self.app.sendEvent(self.gridWidget, event)
    #    return super().eventFilter(obj, event)

    def sizeHint(self):
        return self.app.desktop().screenGeometry().size() 
