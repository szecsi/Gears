import Gears as gears
from .. import * 
from ..Figure.Base import *

class Min(Base) : 

    def applyWithArgs(
            self,
            spass,
            functionName,
            *,
            figure1     : 'First operand. (Figure.*)'
                        =   Figure.Solid( color = 'white' ),
            figure2     : 'Second operand. (Figure.*)'
                        =   Figure.Solid( color = 'white' )
            ) :
        stimulus = spass.getStimulus()
        figure1.apply(spass, functionName + '_op1')
        figure2.apply(spass, functionName + '_op2')

        spass.setShaderFunction( name = functionName, src = self.glslEsc( '''
                vec3 @<pattern>@ (vec2 x, float time){
                    return min( @<pattern>@_op1(x), @<pattern>@_op2(x) ); 
                }
        ''').format( pattern=functionName )  ) 

