import Gears as gears
from .. import * 

class BigDog(Component, gears.SpatialFilter) :

    def __init__(self, **args):
        gears.SpatialFilter.__init__(self, 'BigDog' )
        Component.__init__(self, **args)

    def applyWithArgs(
            self,
            stimulus,
            *,
            sigma1  : 'Standard deviation of 2D Gaussian term 1 [um].'
                    = 8.0,
            weight1 : 'Weight factor for 2D Gaussian term 1, used when adding the two Gaussians.'
                    = -10.0,
            sigma2  : 'Standard deviation of 2D Gaussian term 2 [um].'
                    = 5.0,
            weight2 : 'Weight factor for 2D Gaussian term 2, used when adding the two Gaussians.'
                    = 10.0,
            offset  : 'Constant term added to the weighted sum of the two Gaussians.'
                    = 0.0
            ) :
        self.setShaderVariable("sigma1", sigma1 )
        self.setShaderVariable("weight1",weight1)
        self.setShaderVariable("sigma2", sigma2 )
        self.setShaderVariable("weight2",weight2)
        self.setShaderVariable("offset", offset )
        self.setShaderFunction("kernel", """
            vec4 kernel(vec2 x){
                vec2 sigma = vec2(sigma1, sigma2);
                vec2 weight = vec2(weight1, weight2);
                vec2 a = exp(-dot(x,x) / (2 * sigma * sigma)) / (sigma * 6.283185307179586476925286766559 * sigma);
                float s = dot(a, weight) + offset;
                return vec4(s, 0, s, 0); }
        """)
        self.width_um = sigma1 * 5
        self.height_um = sigma1 * 5
        self.maximum = weight1 / (sigma1 * 6.283185307179586476925286766559 * sigma1) + weight2 / (sigma2 * 6.283185307179586476925286766559 * sigma2)
        self.minimum = self.maximum * -0.125
        stimulus.setSpatialFilter(self)


