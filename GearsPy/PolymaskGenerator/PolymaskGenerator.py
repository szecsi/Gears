from PyQt5.QtGui import (QColor)
from PyQt5.QtOpenGL import (QGLWidget)
import GearsUtils as utils
import numpy as np
try:
    from OpenGL.GL import *
except:
    print ('ERROR: PyOpenGL not installed properly.')

CATMULL_ROM_MTX = np.array([
            [    0,    1,    0,    0],
            [ -0.5,    0,  0.5,    0],
            [    1, -2.5,    2, -0.5],
            [ -0.5,  1.5, -1.5,  0.5]
        ])

class PolymaskGenerator(QGLWidget):
    def __init__(self, parent, winId):
        super().__init__()

        self.selectedControlIndex = None
        self.controlPoints = [
            np.array([0, 0]),
            np.array([0, 0.5]),
            np.array([0.5, 0.5]),
            np.array([0.5, 0])
        ]

        utils.initQGLWidget(self, super(), parent, winId)

        color = QColor (0, 255, 0, 255)
        self.qglClearColor(color)

    def resizeGL(self, w, h):
        self.width = w
        self.height = h
        glViewport(0, 0, w, h)

    def paintGL(self):
        glClear( GL_COLOR_BUFFER_BIT )
        self.vertecies = []
        self.drawCurve()

    def mousePressEvent(self, e):
        mouse = [(e.x()-self.width / 2) * 2, (-e.y()+self.height/2)*2]

        self.selectedControlIndex = None
        for idx, point in enumerate(self.controlPoints):
            p = point.tolist()
            if np.linalg.norm(np.array([p[0]*self.width, p[1]*self.height])-np.array(mouse)) < 10:
                self.selectedControlIndex = idx

    def mouseReleaseEvent(self, e):
        self.selectedControlIndex = None

    def mouseMoveEvent(self, e):
        if self.selectedControlIndex == None:
            return
        mouse = [e.x()/self.width, (self.height-e.y())/self.height]
        mouse = [(i-0.5)*2 for i in mouse]

        self.controlPoints[self.selectedControlIndex] = np.array(mouse)
        self.update()

    def drawCurve(self):
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

        glEnableClientState( GL_VERTEX_ARRAY )
        glVertexPointer( 2, GL_FLOAT, 0, self.vertecies )
        glColor3f(0.0, 0.0, 0.0)
        glDrawArrays( GL_LINE_STRIP, 0, int(len(self.vertecies)/2) )
        glDisableClientState( GL_VERTEX_ARRAY )

    def drawSegment(self, P, precision = 100):
        self.vertecies.extend(P[1].tolist())
        t = 0
        while t < 1:
            t += 1 / precision
            catmull_rom_vec = np.matmul(np.array([
                1, t, t*t, t*t*t
            ]), CATMULL_ROM_MTX)

            ft = np.dot(catmull_rom_vec, P)
            self.vertecies.extend([float(i) for i in ft.tolist()])

    def show(self, controlPoints):
        self.points = controlPoints
        super().show()
