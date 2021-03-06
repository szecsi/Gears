import Gears as gears
from .. import * 
from .SingleShape import *

class PhaseInvertingGrid(SingleShape) :

    def boot(
            self,
            *,
            duration        : 'Stimulus time in frames (unless superseded by duration_s).'
                            =   1,
            duration_s      : 'Stimulus time in seconds (takes precendece over duration given in frames).'
                            =   0,
            name            : 'Stimulus name to display in sequence overview plot.'
                            ='PhaseInvertingGrid',
            color           : 'Color as (r,g,b) triplet or color name.'
                            = 'white',
            direction       : "The motion direction in radians (or 'east', 'northeast', 'north', 'northwest', 'west', 'southwest', 'south', or 'southeast')"
                            = 'east',
            wavelength_um   : 'Sine pattern wavelength (distance between crests) [um].'
                            = 600,
            sineExponent    : 'Cosine exponent (1 for cosine, 0.01 for square signal).'
                            = 1
            ):
        direction = processDirection(direction, self.tb)
        s = math.sin(direction)
        c = math.cos(direction)
        super().boot(name=name, duration=duration, duration_s=duration_s,
                     pattern = Pif.CampbellRobertson(
                            direction = direction,
                            startWavelength = wavelength_um,
                            endWavelength   = wavelength_um,
                            sineExponent    = sineExponent,
                            minContrast     = 1,
                            maxContrast     = 1,
                            ),
                     patternMotion = Motion.DiscreteLinear(
                            jump        =   ( c*wavelength_um*0.5, s*wavelength_um*0.5 ),
                            ),
                     )