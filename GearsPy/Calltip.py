from PyQt5.QtCore import *
from PyQt5.QtGui import *
from PyQt5.QtWidgets import QLabel, QDialog, QWidget, QGridLayout
from PyQt5.Qsci import QsciScintilla, QsciScintillaBase, QsciLexerPython, QsciAPIs

class Calltip(QLabel):

    def __init__(self, editor, lpos, parent=None):
        self.lpos = lpos
        self.editor = editor
        super(Calltip, self).__init__(parent)
        font = QFont()
        font.setFamily('Courier')
        font.setFixedPitch(True)
        font.setPointSize(10)
        self.setFont(font)

        self.setTextFormat( Qt.RichText )
        self.setOpenExternalLinks( False )
        self.linkActivated.connect(self.onLink)

        self.setFocusPolicy(Qt.NoFocus)
        self.window().installEventFilter(self)

    def onLink(self, link):
        if link in self.pdict:
            self.editor.jumpTo(self.pdict[link][1])
        else:
            citem = self.cdict[link]
            valrep = repr(citem[0])
            try :
                if citem[0].startswith("Project.Components.") :
                    valrep = citem[0][len("Project.Components."):] + "()"
            except AttributeError:
                pass
            self.editor.addKeyword( '''{keyword} = {defval} ,'''.format( keyword = link, defval = valrep ) , self.lpos )
            #TODO indent as in code

    #def mousePressEvent(self, e):
    #    super().mousePressEvent(e)
    #    self.close()
        
    def highlight(self, cdict, pdict):
        text = '<TABLE cellpadding=3>'
        for ckw, (cdefault, canno) in sorted(cdict.items()) :
            if ckw in pdict.keys() :
                text += '''
                <TR>
                <TD> <A HREF="{varname}" style="color:blue;">{varname}</A></TD> <TD align=right style="color:black">{val}</TD><TD>{doc}</TD></TR>
                '''.format(
                            varname = ckw, val=pdict[ckw][0], doc=canno)
            #    text = text.replace('>' + key + '</A>', 'style="color:red;">' + key + '</A>')
            else:
                valrep = repr(cdefault)
                try :
                    if cdefault.startswith("Project.Components.") :
                        valrep = cdefault[len("Project.Components."):] + "()"
                except AttributeError:
                    pass
                text += '''
                <TR>
                <TD> <A HREF="{varname}" style="color:red;">{varname}</A></TD> <TD align=right style="color:green">{val}</TD><TD>{doc}</TD></TR>
                '''.format(
                            varname = ckw, val=valrep, doc=canno)
        text += '</TABLE>'
        first = True
        for kw in pdict.keys() :
            if not kw in cdict.keys():
                if first:
                    text += 'unknown keywords: '
                    first = False
                text += kw + ' '
        self.setText(text)
        self.pdict = pdict
        self.cdict = cdict
    
    #def focusOutEvent(self, e):
    #    print('callti[ lsot foxus\n')
    #    self.hide()

    def eventFilter(self, obj, event):
        #print(type(event))
        if event.type() == QEvent.WindowDeactivate:
            #print('dx')
            self.hide()
        return super().eventFilter(obj, event)