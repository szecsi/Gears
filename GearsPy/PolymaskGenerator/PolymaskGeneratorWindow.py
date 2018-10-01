from PyQt5.QtWidgets import (QWidget, QSplitter, QGridLayout, QPushButton)
from PolymaskGenerator.PolymaskGenerator import *

class PolymaskGeneratorWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setMinimumSize(1600, 720)
        self.setMaximumSize(1600, 720)
        self.setGeometry(100, 100, 1600, 720)

        layout = QGridLayout(self)

        self.polymaskGenerator = PolymaskGenerator(self, self.winId())
        layout.addWidget(self.polymaskGenerator, 1, 1, 1, 4)

        right_panel = QWidget(None)
        right_panel_layout = QGridLayout()

        saveButton = QPushButton("Save", right_panel)
        right_panel_layout.addWidget(saveButton, 1, 1, 1, 1)

        right_panel.setLayout(right_panel_layout)
        layout.addWidget(right_panel, 1 ,5, 1, 1)

        self.setLayout(layout)
        self.setWindowTitle("Polymask Generator")