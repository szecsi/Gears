from PyQt5.QtWidgets import (QWidget, QSplitter, QGridLayout, QPushButton, QVBoxLayout, QListWidget)
from PyQt5.QtCore import (Qt)
from PolymaskGenerator.PolymaskGenerator import PolymaskGenerator, PolymaskChangeEvent

class PolymaskGeneratorWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setMinimumSize(1628, 742) # 1600 plus margins
        self.setMaximumSize(1628, 742)
        self.setGeometry(100, 100, 1628, 742)

        layout = QGridLayout(self)
        self.setLayout(layout)
        self.setWindowTitle("Polymask Generator")
        controlPoints = [
            [-0.25, -0.25],
            [-0.25, 0.25],
            [0.25, 0.25],
            [0.25, -0.25]
        ]

        splines = [controlPoints, controlPoints]

        panelStyleSheet = """
        .QWidget {
            border: 2px solid black;
            }
        """

        self.polymaskGenerator = PolymaskGenerator(self, self.winId(), splines, self.dataChanged)
        layout.addWidget(self.polymaskGenerator)
        layout.setColumnStretch(0, 4)

        right_panel = QSplitter(self)
        right_panel.setChildrenCollapsible(False)

        generate_panel = QWidget(right_panel)
        generate_panel.setStyleSheet(panelStyleSheet)
        generate_layout = QVBoxLayout(generate_panel)
        generate_panel.setLayout(generate_layout)

        control_polygon_panel = QWidget(right_panel)
        control_polygon_panel.setStyleSheet(panelStyleSheet)
        control_polygon_layout = QVBoxLayout(control_polygon_panel)
        control_polygon_panel.setLayout(control_polygon_layout)

        control_splines_panel = QWidget(right_panel)
        control_splines_panel.setStyleSheet(panelStyleSheet)
        control_splines_layout = QVBoxLayout(control_splines_panel)
        control_splines_panel.setLayout(control_splines_layout)

        saveButton = QPushButton("Save", right_panel)
        saveButton.clicked.connect(self.save)
        generate_layout.addWidget(saveButton)

        generateButton = QPushButton("Show filled polygons", right_panel)
        generateButton.clicked.connect(self.generateTriangles)
        generate_layout.addWidget(generateButton)
        
        self.cp_list = QListWidget()

        idx = 0
        for cp in controlPoints:
            if len(cp) < 2:
                print("Error: controlPoint size less then 2")
                continue
            self.cp_list.addItem(self.pointToString(idx, cp))
            idx += 1

        self.cp_list.setFixedHeight(100)
        self.cp_list.setCurrentRow(0)
        self.cp_list.currentItemChanged.connect(self.currentPointChanged)
        control_polygon_layout.addWidget(self.cp_list)

        addBeforeButton = QPushButton("Add Controlpoint before selected", right_panel)
        addBeforeButton.clicked.connect(self.addBefore)
        control_polygon_layout.addWidget(addBeforeButton)

        addAfterButton = QPushButton("Add Controlpoint after selected", right_panel)
        addAfterButton.clicked.connect(self.addAfter)
        control_polygon_layout.addWidget(addAfterButton)

        self.curve_list = QListWidget()
        for idx in range(len(splines)):
            self.curve_list.addItem("Spline " + str(idx))
        self.curve_list.setFixedHeight(100)
        self.curve_list.setCurrentRow(0)
        control_splines_layout.addWidget(self.curve_list)

        addSplineButton = QPushButton("Add New Spline", right_panel)
        addSplineButton.clicked.connect(self.addSpline)
        control_splines_layout.addWidget(addSplineButton)

        right_panel.setOrientation(Qt.Vertical)
        right_panel.addWidget(generate_panel)
        right_panel.addWidget(control_polygon_panel)
        right_panel.addWidget(control_splines_panel)
        
        layout.addWidget(right_panel, 0, 1)
        layout.setColumnStretch(1, 1)

    def pointToString(self, idx, point):
        return "P" + str(idx) + " ( " + "{:.4f}".format(point[0]) + ", " + "{:.4f}".format(point[1]) + " )" if idx >=0 else "( " + "{:.4f}".format(point[0]) + ", " + "{:.4f}".format(point[1]) + " )"

    def save(self):
        if self.generateTriangles():
            self.saveFunction(self.polymaskGenerator.fill)
            self.close()

    def addBefore(self):
        self.polymaskGenerator.addBefore(self.cp_list.currentRow())

    def addAfter(self):
        self.polymaskGenerator.addAfter(self.cp_list.currentRow())

    def generateTriangles(self):
        return self.polymaskGenerator.generateTriangles()

    def set(self, saveFunction):
        self.saveFunction = saveFunction

    def dataChanged(self, type, index, value = None):
        if type == PolymaskChangeEvent.SelectionChanged:
            self.cp_list.setCurrentRow(index)
        elif type == PolymaskChangeEvent.ItemChanged:
            item_text = self.cp_list.item(index).text()
            last_idx = item_text.index("(")
            item_text = item_text[:last_idx]
            self.cp_list.item(index).setText(item_text + self.pointToString(-1, value))
        elif type == PolymaskChangeEvent.ItemAdded:
            self.cp_list.insertItem(index, self.pointToString(self.cp_list.count(), value))

    def currentPointChanged(self):
        self.polymaskGenerator.currentPointChanged(self.cp_list.currentRow())

    def closeEvent(self, event):
        self.saveFunction = None
        event.accept()

    def addSpline(self):
        self.polymaskGenerator.addSpline()
