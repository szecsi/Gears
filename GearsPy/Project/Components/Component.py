import traceback
import warnings
import re
from SequenceError import SequenceError

class Component() : 
    def __init__(self, **args):
        self.args = args
        self.tb = traceback.extract_stack()
        try :
            while not self.tb[-1][0].endswith('pyx') :
                self.tb.pop()
        except IndexError:
            self.tb = None
            #raise SequenceError('Component not created in a .pyx file!', traceback.extract_stack())

    def __repr__(self):
        return type(self).__name__ + str(self.args)

    def apply(self, spass) :
        #if not self.tb[-2][0].endswith('pyx') :
        if self.tb == None :
            self.tb = spass.tb
        self.applyWithArgs(spass, **self.args)

    def warn(self, message):
        warnings.warn('Warning: <BR>' 
                + self.tb[-1][0] + '(' + str(self.tb[-1][1]) + '):<BR> in function "' 
                + self.tb[-1][2] + '":<BR> ' + self.tb[-1][3] + '<BR><BR>' + message )

    def multiple_replace(self, string, rep_dict):
        pattern = re.compile("|".join([re.escape(k) for k in rep_dict.keys()]), re.M)
        return pattern.sub(lambda x: rep_dict[x.group(0)], string)

    def glslEsc(self, string):
        return self.multiple_replace(string, {'{' : '{{', '}' : '}}', '@<' : '{', '>@' : '}', '`' : '{X}_' })