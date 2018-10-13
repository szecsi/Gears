from PyQt5.QtGui import (QColor)
from PyQt5.QtOpenGL import (QGLWidget)
import GearsUtils as utils
import numpy as np
from Gears import CDT
from enum import Enum
try:
    from OpenGL.GL import *
except:
    print ('ERROR: PyOpenGL not installed properly.')

class PolymaskChangeEvent(Enum):
    SelectionChanged = 0
    ItemChanged = 1
    ItemAdded = 2
    ItemRemoved = 3

CATMULL_ROM_MTX = np.array([
            [    0,    1,    0,    0],
            [ -0.5,    0,  0.5,    0],
            [    1, -2.5,    2, -0.5],
            [ -0.5,  1.5, -1.5,  0.5]
        ])

class PolymaskGenerator(QGLWidget):
    def __init__(self, parent, winId, controlPoints, dataChanged):
        super().__init__()

        self.selectedControlIndex = None
        self.dataChanged = dataChanged
        self.controlPoints = [np.array(a) for a in controlPoints]

        utils.initQGLWidget(self, super(), parent, winId)

        color = QColor (255, 255, 255, 255)
        self.qglClearColor(color)

        self.fill = None

    def resizeGL(self, w, h):
        self.setFixedSize(1280, 720)
        glViewport(0, 0, 1280, 720)

    def paintGL(self):
        glClear( GL_COLOR_BUFFER_BIT )
        glEnableClientState( GL_VERTEX_ARRAY )
        self.drawCurve()
        self.drawControlPoints()
        glDisableClientState( GL_VERTEX_ARRAY )

    def drawControlPoints(self):
        cp_list = [ c for vec in [i.tolist() for i in self.controlPoints] for c in vec ]
        glVertexPointer( 2, GL_FLOAT, 0, cp_list)
        glColor3f(1.0, 0.0, 0.0)
        glPointSize(10)
        glDrawArrays( GL_POINTS, 0, int(len(self.controlPoints)) )

    def mousePressEvent(self, e):
        mouse = [(e.x()-self.width() / 2) * 2, (-e.y()+self.height()/2)*2]

        self.selectedControlIndex = None
        for idx, point in enumerate(self.controlPoints):
            p = point.tolist()
            if np.linalg.norm(np.array([p[0]*self.width(), p[1]*self.height()])-np.array(mouse)) < 10:
                self.selectedControlIndex = idx
                break

        if self.selectedControlIndex != None:
            self.onDataChanged(PolymaskChangeEvent.SelectionChanged, self.selectedControlIndex)

    def mouseReleaseEvent(self, e):
        self.selectedControlIndex = None

    def mouseMoveEvent(self, e):
        if self.selectedControlIndex == None:
            return
        mouse = [e.x()/self.width(), (self.height()-e.y())/self.height()]
        mouse = [(i-0.5)*2 for i in mouse]

        if not next((True for item in self.controlPoints if np.array_equal(item, np.array(mouse))), False):
            self.controlPoints[self.selectedControlIndex] = np.array(mouse)
            self.onDataChanged(PolymaskChangeEvent.ItemChanged, self.selectedControlIndex, mouse)
            self.update()

    def drawCurve(self):
        self.vertecies = []
        for idx, val in enumerate(self.controlPoints):
            i0 = idx - 1
            i2 = idx + 1
            i3 = idx + 2
            if i2 >= len(self.controlPoints):
                i2 = 0
                i3 = 1
            elif i3 >= len(self.controlPoints):
                i3 = 0

            self.drawSegment(np.array([
                self.controlPoints[i0],
                val,
                self.controlPoints[i2],
                self.controlPoints[i3]
            ]))

        if self.fill:
            glVertexPointer( 2, GL_FLOAT, 0, self.fill )
            glColor3f(0.8, 0.8, 0.8)
            glDrawArrays( GL_TRIANGLES, 0, int(len(self.fill)/2) )

        glVertexPointer( 2, GL_FLOAT, 0, self.vertecies )
        glColor3f(0.0, 0.0, 0.0)
        glDrawArrays( GL_LINE_STRIP, 0, int(len(self.vertecies)/2) )

    def drawSegment(self, P, precision = 50):
        self.vertecies.extend(P[1].tolist())
        t = 1 / precision
        while t < 0.99999999999999:
            catmull_rom_vec = np.matmul(np.array([
                1, t, t*t, t*t*t
            ]), CATMULL_ROM_MTX)

            ft = np.dot(catmull_rom_vec, P)
            self.vertecies.extend([float(i) for i in ft.tolist()])
            t += 1 / precision

    def addBefore(self, index):
        if not next((True for item in self.controlPoints if np.array_equal(item, np.array([0,0]))), False):
            self.controlPoints.insert(index, np.array([0,0]))
            self.onDataChanged(PolymaskChangeEvent.ItemAdded, index, [0,0])
            self.update()

    def addAfter(self, index):
        if not next((True for item in self.controlPoints if np.array_equal(item, np.array([0,0]))), False):
            self.controlPoints.insert(index+1, np.array([0,0]))
            self.onDataChanged(PolymaskChangeEvent.ItemAdded, index+1, [0,0])
            self.update()

    def generateTriangles(self):
        cdt = CDT(self.vertecies)
        cdt.triangulate()
        self.fill = cdt.get_triangles()
        self.update()

    def onDataChanged(self, type, index, value = None):
        self.fill = None
        self.dataChanged(type, index, value)