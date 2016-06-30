import Gears as gears
from .. import * 
from ..Figure.Base import *

class Instanced(Base) : 

    def applyWithArgs(
            self,
            spass,
            functionName,
            *,
            figures             : 'List of figures the instances may use. (list of Figure.*)'
                                = [],
            motions             : 'List of motions the instances may use. (list of Motion.*)'
                                = []
            ) :
        ffunc = '''
            in vec2 figmotid;
            vec3 @<X>@ (vec2 x, float time){
        '''
        stimulus = spass.getStimulus()
        i = 0
        for fig in figures :
            fig.apply(spass, functionName + '_f' + str(i))
            ffunc += 'if(figmotid.x < {ipi}) return `f{i}(x, time);'.format(ipi = (i+1) / len(figures), i=i)
            i += 1
        ffunc += '}'

        mfunc = '''
            vec2 motionTransform(vec2 x, float time, float motid, ivec2 iid) { 
        '''
        j = 0
        for mot in motions :
            mot.applyForward(spass, functionName + '_m' + str(j))
            mfunc += 'if(motid < {ipi}) return `m{i}(time) * vec3(x, 1);'.format(ipi = (j+1) / len(motions), i=j)
            j += 1
        mfunc += '}'

        spass.setMotionTransformFunction( self.glslEsc( mfunc ).format( X=functionName ) )

        spass.setShaderFunction( name = functionName, src = self.glslEsc( ffunc ).format( X=functionName )  ) 

