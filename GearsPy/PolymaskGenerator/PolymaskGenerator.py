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

_catmullShader = None
_vertexArray = None
_vertexBuffer = None

class PolymaskGenerator(QGLWidget):
    def __init__(self, parent, winId):
        super().__init__()

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
        self.vertecies = []
        self.drawCurve()

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
            ]),1)

        glEnableClientState( GL_VERTEX_ARRAY )
        glVertexPointer( 2, GL_FLOAT, 0, self.vertecies )
        glDrawArrays( GL_LINE_STRIP, 0, int(len(self.vertecies)/2) )
        glDisableClientState( GL_VERTEX_ARRAY )

    def drawSegment(self, P, precision = 10):
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
