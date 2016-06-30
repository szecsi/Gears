import Gears as gears
from .. import * 
from .Base import *

class Cosine(Base) : 

    def applyWithArgs(
            self,
            spass,
            functionName,
            *,
            displayName     : 'The name that apears on theexperience timelne plot. If None, this temporal modulation function is not displayed. '
                            = None,
            brightColor     : 'Modulating color at unit modulated intensity.'
                            = (1, 1, 1),
            darkColor       : 'Modulating color at zero modulated intensity.'
                            = (0, 0, 0),
            intensity       : 'Base intensity before modulation'
                            = 0.5,
            wavelength_s    : 'Starting wavelength [s].'
                            = 0,
            endWavelength_s : 'Ending wavelength [s].'
                            = 0,
            exponent        : 'Cosine exponent (1 for cosine, 0.01 for square signal).'
                            = 1,
            phase           : 'Phase shift [radian]'
                            = 0,
            amplitude       : 'Starting intensity modulation amplitude (intensity will be in [intensity-amplitude, intensity+amplitude], starting at the minimum).'
                            = 0.5,
            endAmplitude    : 'Ending intensity modulation amplitude (defaults to amplitude). Amplitude is linearly interpolated over time.'
                            = -1
            ) :

        if displayName:
            spass.registerTemporalFunction(functionName, displayName)

        brightColor = processColor(brightColor, self.tb)
        darkColor = processColor(darkColor, self.tb)

        stimulus = spass.getStimulus()
        if max(brightColor) - min(brightColor) > 0.03 or max(darkColor) - min(darkColor) > 0.03 :
            stimulus.enableColorMode()

        duraSec = spass.getDuration_s()

        spass.setShaderColor(       name = functionName+"_brightColor", red = brightColor[0], green=brightColor[1], blue=brightColor[2] )
        spass.setShaderColor(       name = functionName+"_darkColor", red = darkColor[0], green=darkColor[1], blue=darkColor[2] )
        spass.setShaderVariable(    name = functionName+"_duration_s", value = duraSec )
        spass.setShaderVariable(    name = functionName+"_exponent", value = exponent )
        spass.setShaderVariable(    name = functionName+"_offset", value = intensity )
        spass.setShaderVariable(    name = functionName+"_phase", value = phase )
        spass.setShaderVector(      name = functionName+'_wavelength', x = wavelength_s / duraSec, y = (endWavelength_s / duraSec if endWavelength_s > 0 else wavelength_s / duraSec) )
        spass.setShaderVector(      name = functionName+'_amplitude', x = amplitude, y = (0 if endAmplitude < -0.1 else ((endAmplitude - amplitude) / duraSec)) )
        
        spass.setShaderFunction( name = functionName, src = self.glslEsc( '''
                vec3 @<X>@(float time){ 
                    float t = time / @<X>@_duration_s;
                    float q = log(- @<X>@_wavelength.x * t + @<X>@_wavelength.x + @<X>@_wavelength.y * t) / (@<X>@_wavelength.x + @<X>@_wavelength.y);
        		    q -= log(@<X>@_wavelength.x) / (@<X>@_wavelength.x + @<X>@_wavelength.y);
                    if(abs(@<X>@_wavelength.x - @<X>@_wavelength.y) < 0.000001)
                        q = time / @<X>@_wavelength.x / @<X>@_duration_s;
                    float currentAmplitude = @<X>@_amplitude.x + time * @<X>@_amplitude.y;
                    float s = cos(q * 6.283185307179586476925286766559 + `phase);
                    if(s<0)
                        s = -pow(-s, @<X>@_exponent);
                    else
                        s = pow(s, @<X>@_exponent);
                    return mix(@<X>@_darkColor, @<X>@_brightColor, -s*currentAmplitude + @<X>@_offset);
                    }
        ''' ).format( X = functionName ) )