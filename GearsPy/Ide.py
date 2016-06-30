from PyQt5.QtCore import *
from PyQt5.QtGui import *
from PyQt5.QtWidgets import QApplication, QLabel, QDialog, QWidget, QGridLayout, QPushButton
from PyQt5.Qsci import QsciScintilla, QsciScintillaBase, QsciLexerPython, QsciAPIs

from Editor import Editor
from SequenceLoader import *

class Ide(QWidget):

    def __init__(self, sequencePath, browser, errline=1, parent=None):
        super().__init__(parent)
        self.sequencePath = sequencePath
        self.browser = browser
        grid = QGridLayout()

        self.reloadButton = QPushButton('Save script and load sequence', self)
        self.reloadButton.clicked.connect(self.reload)
        grid.addWidget(self.reloadButton, 1, 1, 1, 1)

        self.discardButton = QPushButton('Discard changes', self)
        self.discardButton.clicked.connect(self.discard)
        grid.addWidget(self.discardButton, 1, 2, 1, 1)
    
        self.editor = Editor(sequencePath, errline, self)
        grid.addWidget(self.editor, 2, 1, 10, 2)
        self.setLayout(grid)

    def reload(self, e):
        self.editor.save()
        self.close()
        if loadSequence(self.sequencePath, self.browser):
            self.browser.launcherWindow.start(self.browser.browserWindow, self.sequencePath)
            QApplication.instance().processEvents()
            self.browser.browserWindow.hide()

    def discard(self, e):
        self.close()
