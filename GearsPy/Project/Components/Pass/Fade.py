import Gears as gears
from .. import * 
import traceback
import inspect
from .Generic import *

class Fade(Generic) : 

    def __init__(self, **args):
        super().__init__(**args)

    def boot(
            self,
            *,
            name            : 'Pass name to display in sequence overview plot.'
                            = 'Generic',
            duration        : 'Pass time in frames. Defaults to stimulus duration. Superseded by duration_s is given.'
                            = 0,
            duration_s      : 'Pass time in seconds (takes precendece over duration given in frames).'
                            = 0,
            shape           : 'Component for shape. (Shape.*)'
                            = Figure.Solid()     ,
            shapeMotion     : 'Motion component for shape animation. (Motion.*)'
                            = Motion.Linear()       ,
            pattern1        : 'First pattern component participating in the fading. (Pattern.*)'
                            = Figure.Solid()       ,
            pattern2        : 'Second pattern component participating in the fading. (Pattern.*)'
                            = Figure.Solid()       ,
            patternMotion   : 'Motion component for pattern animation. (Motion.*)'
                            = Motion.Linear()       ,
            modulation      : 'Component for temporal modulation. (Modulation.*)'
                            = Modulation.Linear()   ,
            warp            : 'Component for image distortion. (Warp.*)'
                            = Warp.Nop()            
            ):

        if not 'displayName' in modulation.args :
            modulation.args['displayName'] = 'Fade factor'
        
        rootFigure = ((shape << shapeMotion << modulation) ** (pattern1 << patternMotion, pattern2 << patternMotion)) << warp

        super().boot(name=name, duration=duration, duration_s=duration_s,
            figure = rootFigure,
        )



#
#import Gears as gears
#from .. import * 
#import traceback
#import inspect
#from .Base import *
#
#class Fade(Base) : 
#
#    def __init__(self, **args):
#        super().__init__(**args)
#
#    def boot(
#            self,
#            *,
#            name            : 'Pass name to display in sequence overview plot.'
#                            = 'Generic',
#            duration        : 'Pass time in frames. Defaults to stimulus duration. Superseded by duration_s is given.'
#                            = 0,
#            duration_s      : 'Pass time in seconds (takes precendece over duration given in frames).'
#                            = 0,
#            shape           : 'Component for shape. (Shape.*)'
#                            = Figure.Solid()     ,
#            shapeMotion     : 'Motion component for shape animation. (Motion.*)'
#                            = Motion.Linear()       ,
#            pattern1        : 'First pattern component participating in the fading. (Pattern.*)'
#                            = Figure.Solid()       ,
#            pattern2        : 'Second pattern component participating in the fading. (Pattern.*)'
#                            = Figure.Solid()       ,
#            patternMotion   : 'Motion component for pattern animation. (Motion.*)'
#                            = Motion.Linear()       ,
#            modulation      : 'Component for temporal modulation. (Modulation.*)'
#                            = Modulation.Linear()   ,
#            warp            : 'Component for image distortion. (Warp.*)'
#                            = Warp.Nop()            
#            ):
#        self.name                =      name
#        self.duration            =      duration
#        sequence = self.getSequence()
#        stimulus = self.getStimulus()
#        self.duration = duration
#        if(duration == 0):
#            self.duration = stimulus.getDuration()
#        if(duration_s != 0):
#            self.duration = int(duration_s // sequence.getFrameInterval_s() + 1)
#        
#        shape        .apply(self)
#        shapeMotion  .apply(self)
#        pattern1.args['patternFunctionName'] = 'pattern1'
#        pattern1     .apply(self)
#        pattern2.args['patternFunctionName'] = 'pattern2'
#        pattern2     .apply(self)
#        patternMotion.apply(self, pattern=True)
#        modulation   .apply(self)
#        warp         .apply(self)
#
#
#        self.stimulusGeneratorShaderSource = """
#    		in vec2 pos;
#	    	out vec4 outcolor;
#		    void main() { 
#                vec3 shapeMask = shape( warp(pos - position(time)));
#                vec2 warpedPos = warp(pos - patternPosition(time));
#                outcolor = vec4(
#                mix(
#                    pattern1( shapeMask, warpedPos),
#                    pattern2( shapeMask, warpedPos),
#                    intensity(time))
#                , 0); 
#                if(swizzleForFft)
#                    outcolor = vec4(outcolor.g, 0, outcolor.b, 0);
#                }
#		"""
#
#