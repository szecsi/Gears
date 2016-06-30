import Gears as gears
from .. import * 
from .SingleShape import *

class Blank(SingleShape) :

    def __init__(self, **args):
        super().__init__( **args )

    def boot(self,
            *,
            duration    : 'Stimulus time in frames.' 
                        = 1,
            duration_s  : 'Stimulus time in seconds (takes precendece over duration given in frames).'
                        = 0,
            name        : 'Stimulus name to display in sequence overview plot.'
                        = '.',
            color       : 'Base color as an rgb triplet or color name.'
                        = "white",
            intensity   : 'Modulates base color (0 - black for any color). Value plotted in sequence overview.'
                        = 0,
            temporalFilter = Temporal.Nop()
            ):
        if name=='.' and intensity!=0 :
            name = ' {intensity:.1f}'.format(intensity=intensity) 
        super().boot(name=name, duration=duration, duration_s=duration_s,
                     modulation =   Modulation.Linear( brightColor=color, intensity=intensity),
                     signal     =   Signal.NoTicks(), 
                     temporalFilter = temporalFilter,
                     )

