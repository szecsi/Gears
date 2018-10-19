from PyQt5.QtGui import (QColor)
from PyQt5.QtOpenGL import (QGLWidget)
from PyQt5.QtWidgets import (QErrorMessage)
import GearsUtils as utils
import numpy as np
from math import sqrt
from Gears import CDT
from enum import Enum
try:
    from OpenGL.GL import *
except:
    print ('ERROR: PyOpenGL not installed properly.')

def get_lines_intersection(P1, P2, P3, P4):
    a1 = P2[1] - P1[1]
    b1 = P1[0] - P2[0]
    d1 = (P1[0] - P2[0]) * P1[1] - (P1[1] - P2[1]) * P1[0]

    a2 = P4[1] - P3[1]
    b2 = P3[0] - P4[0]
    d2 = (P3[0] - P4[0]) * P3[1] - (P3[1] - P4[1]) * P3[0]

    # x = (b2d1 - b1d2)/(a1b2 - a2b1)
    # y = (a1d2 - a2d1)/(a1b2 - a2b1)

    nx = (a1 * b2 - a2 * b1)
    ny = (a1 * b2 - a2 * b1)
    if nx == 0 or ny == 0: # check 0 division
        return None

    x = (b2 * d1 - b1 * d2) / nx
    y = (a1 * d2 - a2 * d1) / ny
    
    return [x,y]

def check_point_on_section(P, P1, P2):
    if P1 == P or P2 == P: # intersection at the end
        return True
    if P1[0] == P[0] or P2[0] == P1[0]: # do not devide with zero
        return False
    m1 = P2[1] - P1[1] / (P2[0] - P1[0])
    m2 = P[1] - P1[1] / (P[0] - P1[0])

    if (m1 < 0 and m2 > 0) or (m1 > 0 and m2 < 0):
        return False
    
    return sqrt((P[0]-P1[0])*(P[0]-P1[0]) + (P[1]-P1[1])*(P[1]-P1[1])) < sqrt((P2[0]-P1[0])*(P2[0]-P1[0]) + (P2[1]-P1[1])*(P2[1]-P1[1]))

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
    def __init__(self, parent, winId, splines, dataChanged):
        super().__init__()

        self.selectedControlIndex = None
        self.last_selected = 0
        self.dataChanged = dataChanged
        self.splines = []
        for s in splines:
            self.splines.append([np.array(a) for a in s])

        utils.initQGLWidget(self, super(), parent, winId)

        color = QColor (255, 255, 255, 255)
        self.qglClearColor(color)

        self.fill = {}

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
        cp_list = [ c for vec in [i.tolist() for i in [s for sp in self.splines for s in sp]] for c in vec ]
        glVertexPointer( 2, GL_FLOAT, 0, cp_list)
        glColor3f(1.0, 0.0, 0.0)
        glPointSize(10)
        glDrawArrays( GL_POINTS, 0, int(len([i for i in [s for sp in self.splines for s in sp]])) )
        glColor3f(0.0, 0.0, 1.0)
        glDrawArrays( GL_POINTS, self.last_selected, 1 )

    def mousePressEvent(self, e):
        mouse = [(e.x()-self.width() / 2) * 2, (-e.y()+self.height()/2)*2]

        self.selectedControlIndex = None
        for idx, point in enumerate(self.splines[0]): #TODO check all splines
            p = point.tolist()
            if np.linalg.norm(np.array([p[0]*self.width(), p[1]*self.height()])-np.array(mouse)) < 10:
                self.selectedControlIndex = idx
                self.last_selected = idx
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

        if not next((True for item in self.splines[0] if np.array_equal(item, np.array(mouse))), False): # TODO all splines
            self.splines[0][self.selectedControlIndex] = np.array(mouse) # TODO all splines
            self.onDataChanged(PolymaskChangeEvent.ItemChanged, self.selectedControlIndex, mouse)
            self.update()

    def drawCurve(self):
        self.vertecies = []
        for s_idx in range(len(self.splines)):
            if s_idx in self.fill:
                glVertexPointer( 2, GL_FLOAT, 0, self.fill[s_idx] )
                glColor3f(0.8, 0.8, 0.8)
                glDrawArrays( GL_TRIANGLES, 0, int(len(self.fill[s_idx])/2) )

        for s in self.splines:
            current_vertecies = []
            for idx, val in enumerate(s):
                i0 = idx - 1
                i2 = idx + 1
                i3 = idx + 2
                if i2 >= len(s):
                    i2 = 0
                    i3 = 1
                elif i3 >= len(s):
                    i3 = 0

                current_vertecies.extend(self.drawSegment(np.array([
                    s[i0],
                    val,
                    s[i2],
                    s[i3]
                ])))
            current_vertecies.extend(s[0].tolist())

            glVertexPointer( 2, GL_FLOAT, 0, current_vertecies ) # TODO right vertecies
            glColor3f(0.0, 0.0, 0.0)
            glDrawArrays( GL_LINE_STRIP, 0, int(len(current_vertecies)/2) )

            self.vertecies.append(current_vertecies)

    def drawSegment(self, P, precision = 50):
        vertecies = []
        vertecies.extend(P[1].tolist())
        t = 1 / precision
        while t < 0.99999999999999:
            catmull_rom_vec = np.matmul(np.array([
                1, t, t*t, t*t*t
            ]), CATMULL_ROM_MTX)

            ft = np.dot(catmull_rom_vec, P)
            vertecies.extend([float(i) for i in ft.tolist()])
            t += 1 / precision

        return vertecies

    def addBefore(self, index):
        before = index - 1
        if before < 0:
            before = len(self.splines[0])-1

        new_point = (self.splines[0][before] + self.splines[0][index]) / 2
        self.splines[0].insert(index, new_point)
        self.onDataChanged(PolymaskChangeEvent.ItemAdded, index, new_point.tolist())
        self.update()

    def addAfter(self, index):
        if not next((True for item in self.splines[0] if np.array_equal(item, np.array([0,0]))), False):
            self.splines[0].insert(index+1, np.array([0,0]))
            self.onDataChanged(PolymaskChangeEvent.ItemAdded, index+1, [0,0])
            self.update()

    def generateTriangles(self):
        self.fill = {}
        for idx, vert in enumerate(self.vertecies):
            if self.checkIntersection(vert):
                box = QErrorMessage(self)
                box.showMessage("Intersection in one curve is not supported!")
                return False
            cdt = CDT(vert[:-2]) # do not add last point again, poly2tri crashes if there is the same point twice
            cdt.triangulate()
            self.fill[idx] = cdt.get_triangles()
        self.update()
        return True

    def checkIntersection(self, p):
        i = 0
        size = len(p)
        # to_insert = {}
        # to_insert_keys = []
        while i < size-6:
            j = i + 4
            while j < size-2:
                if i == 0 and j== size-4: # first segment is next to the last segment
                    j += 2
                    continue
                P1 = [p[i], p[i+1]]
                P2 = [p[i+2], p[i+3]]
                P3 = [p[j], p[j+1]]
                P4 = [p[j+2], p[j+3]]
                I = get_lines_intersection(P1, P2, P3, P4)
                if I != None:
                    if check_point_on_section(I, P1, P2) and check_point_on_section(I, P3, P4):
                        return True
                        #xto_insert[i] = I
                        #to_insert_keys.append(i)
                j += 2
            i += 2

        # to_insert_keys.sort(reverse=True)
        # for i in to_insert_keys:
        #     self.vertecies.insert(i, to_insert[i][1])
        #     self.vertecies.insert(i, to_insert[i][0])
        return False

    def onDataChanged(self, type, index, value = None):
        self.fill = {}
        self.dataChanged(type, index, value)

    def currentPointChanged(self, idx):
        self.last_selected = idx
        self.update()