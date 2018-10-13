from PyQt5.QtWidgets import (QWidget, QSplitter, QGridLayout, QPushButton, QVBoxLayout, QListWidget)
from PolymaskGenerator.PolymaskGenerator import PolymaskGenerator, PolymaskChangeEvent

class PolymaskGeneratorWindow(QWidget):
    def __init__(self, controlPoints = None):
        super().__init__()
        self.setMinimumSize(1628, 742) # 1600 plus margins
        self.setMaximumSize(1628, 742)
        self.setGeometry(100, 100, 1628, 742)

        layout = QGridLayout(self)
        self.setLayout(layout)
        self.setWindowTitle("Polymask Generator")

        if controlPoints == None:
            controlPoints = [
                [-0.25, -0.25],
                [-0.25, 0.25],
                [0.25, 0.25],
                [0.25, -0.25]
            ]

        self.polymaskGenerator = PolymaskGenerator(self, self.winId(), controlPoints, self.dataChanged)
        layout.addWidget(self.polymaskGenerator)
        layout.setColumnStretch(0, 4)

        right_panel = QWidget(self)
        right_panel_layout = QVBoxLayout(right_panel)
        right_panel.setLayout(right_panel_layout)

        saveButton = QPushButton("Save", right_panel)
        right_panel_layout.addWidget(saveButton)

        generateButton = QPushButton("Show filled polygon", right_panel)
        generateButton.clicked.connect(self.generateTriangles)
        right_panel_layout.addWidget(generateButton)
        
        self.cp_list = QListWidget()

        idx = 0
        for cp in controlPoints:
            if len(cp) < 2:
                print("Error: controlPoint size less then 2")
                continue
            self.cp_list.addItem(self.pointToString(idx, cp))
            idx += 1

        self.cp_list.setFixedHeight(200)
        self.cp_list.setCurrentRow(0)
        right_panel_layout.addWidget(self.cp_list)

        addBeforeButton = QPushButton("Add Controlpoint before selected", right_panel)
        addBeforeButton.clicked.connect(self.addBefore)
        right_panel_layout.addWidget(addBeforeButton)

        addAfterButton = QPushButton("Add Controlpoint after selected", right_panel)
        addAfterButton.clicked.connect(self.addAfter)
        right_panel_layout.addWidget(addAfterButton)

        layout.addWidget(right_panel, 0, 1)
        layout.setColumnStretch(1, 1)

    def pointToString(self, idx, point):
        return "P" + str(idx) + " ( " + "{:.4f}".format(point[0]) + ", " + "{:.4f}".format(point[1]) + " )"

    def addBefore(self):
        self.polymaskGenerator.addBefore(self.cp_list.currentRow())

    def addAfter(self):
        self.polymaskGenerator.addAfter(self.cp_list.currentRow())

    def generateTriangles(self):
        self.polymaskGenerator.generateTriangles()

    def dataChanged(self, type, index, value = None):
        if type == PolymaskChangeEvent.SelectionChanged:
            self.cp_list.setCurrentRow(index)
        elif type == PolymaskChangeEvent.ItemChanged:
            self.cp_list.item(index).setText(self.pointToString(index, value))
        elif type == PolymaskChangeEvent.ItemAdded:
            self.cp_list.insertItem(index, self.pointToString(index, value))