import sys
from platform import system
import Gears as gears
from PyQt5.QtOpenGL import (QGLWidget, QGLFormat, QGLContext)

def initQGLWidget(self, base, parent, winId):
    format = QGLFormat()
    format.setSwapInterval(1)
    if system() == 'Windows':
        base.__init__(format, parent)
        self.makeCurrent()
        gears.shareCurrent()
    elif system() == 'Linux':
        gears.shareCurrent( int(winId) )
        super().__init__(QGLContext.currentContext(), parent)
        self.makeCurrent()
    else:
        print("Not supported platform: " + system())
        sys.exit(1)