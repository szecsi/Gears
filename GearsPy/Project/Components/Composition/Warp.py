import Gears as gears
from .. import * 
from ..Figure.Base import *

class Warp(Base) : 

    def applyWithArgs(
            self,
            spass,
            functionName,
            *,
            figure      : 'Figure to warp. (Figure.*)'
                        =   Figure.Solid( color = 'white' ),
            warp        : 'Warp component. (Warp.*)'
                        =   Warp.Nop( )
            ) :
        stimulus = spass.getStimulus()
        warp.apply(spass,   functionName + '_warp')
        figure.apply(spass, functionName + '_warped')

        spass.setShaderFunction( name = functionName, src = self.glslEsc( '''
                vec3 @<X>@ (vec2 x, float time){
                    return `warped(  `warp(x), time ); 
                }
        ''').format( X=functionName )  ) 

