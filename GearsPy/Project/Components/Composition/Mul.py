import Gears as gears
from .. import * 
from ..Figure.Base import *

class Mul(Base) : 

    def applyWithArgs(
            self,
            spass,
            functionName,
            *,
            shape               : 'Shape pattern. (Pattern.*)'
                                = Figure.Spot(),
            pattern1           : 'Pattern component where shape is zero. (Pattern.*)'
                                = Figure.Solid( color = 'white' ),
            pattern2           : 'Pattern component where shape is one. (Pattern.*)'
                                = Figure.Solid( color = 'white' )
            ) :
        stimulus = spass.getStimulus()
        pattern1.apply(spass, functionName + '_op1')
        pattern2.apply(spass, functionName + '_op2')

        spass.setShaderFunction( name = functionName, src = self.glslEsc( '''
                vec3 @<pattern>@ (vec2 x, float time){
                    return  @<pattern>@_op1(x, time) * @<pattern>@_op2(x, time); 
                }
        ''').format( pattern=functionName )  ) 

