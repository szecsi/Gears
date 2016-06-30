#pragma once

#include "event/Base.h"

#pragma once

#include "event/Base.h"
#include <windowsx.h>

namespace Gears {
	namespace Event {

		class Wheel : public Base
		{
			Wheel(uint message, uint wParam, uint lParam)
				:Base(message, wParam, lParam)
			{
				x = GET_X_LPARAM(lParam);
				y = GET_Y_LPARAM(lParam);
				dX = GET_X_LPARAM(wParam);
				dY = GET_Y_LPARAM(wParam);
			}
		public:
			GEARS_SHARED_CREATE_WITH_GETSHAREDPTR(Wheel);

			uint x;
			uint y;
			int dX;
			int dY;


		public:
			uint globalX(){ return x; }
			uint globalY(){ return y; }
			int deltaX(){ return dX; }
			int deltaY(){ return dY; }


			static uint typeId;
		};

	}
}