#include "stdafx.h"
#include "openCLCore.h"

namespace ImageHelper
{

#ifdef _DEBUG
	void printPixel( float* img, unsigned& idx, unsigned channels, bool complex )
	{
		std::cout << "(";
		for ( unsigned k = 0; k < channels; k++ )
		{
			std::cout << img[idx++];
			if ( complex )
				std::cout << "+" << img[idx++] << "i";
			if ( k < channels - 1 )
				std::cout << ", ";
		}
		std::cout << ") ";
	}
#endif

	void printImg( float* img, unsigned w, unsigned h, const char* name, unsigned channels, bool complex, unsigned pad )
	{
#ifdef _DEBUG
		if ( w > 10 )
			return;
		unsigned idx = 0;
		unsigned rowWidth = complex ? w / 2 + 1 : w;
		std::cout << name << ": " << std::endl;
		for ( unsigned i = 0; i < h; i++ )
		{
			for ( unsigned j = 0; j < rowWidth; j++ )
			{
				printPixel( img, idx, channels, complex );
			}
			if ( pad )
			{
				std::cout << "| ";
				for ( unsigned k = 0; k < pad; k++ )
				{
					printPixel( img, idx, channels, complex );
				}
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
		std::cout << std::endl;
#endif
	}
}