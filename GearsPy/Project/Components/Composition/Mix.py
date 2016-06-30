import Gears as gears
from .. import * 
from ..Figure.Base import *

class Mix(Base) : 

    def applyWithArgs(
            self,
            spass,
            functionName,
            *,
            shape               : 'Shape pattern. (Pattern.*)'
                                = Figure.Spot(),
            backgroundPattern   : 'Pattern component where shape is zero. (Pattern.*)'
                                = Figure.Solid( color = 'black' ),
            foregroundPattern   : 'Pattern component where shape is one. (Pattern.*)'
                                = Figure.Solid( color = 'white' )
            ) :
        stimulus = spass.getStimulus()
        backgroundPattern.apply(spass, functionName + '_background')
        foregroundPattern.apply(spass, functionName + '_foreground')
        shape.apply(spass, functionName=functionName + '_shape')

        spass.setShaderFunction( name = functionName, src = self.glslEsc( '''
                vec3 @<X>@ (vec2 x, float time){
                    return mix( `background(x, time), `foreground(x, time), `shape(x, time)); 
                }
        ''').format( X=functionName )  ) 

