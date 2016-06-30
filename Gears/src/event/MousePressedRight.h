#pragma once

#include "event/Base.h"
#include <windowsx.h>

namespace Gears {
	namespace Event {

		class MousePressedRight : public Base
		{
			MousePressedRight(uint message, uint wParam, uint lParam)
				:Base(message, wParam, lParam)
			{
				x = GET_X_LPARAM(lParam);
				y = GET_Y_LPARAM(lParam);

			}
		public:
			GEARS_SHARED_CREATE_WITH_GETSHAREDPTR(MousePressedRight);

			uint x;
			uint y;

		public:
			uint globalX(){ return x; }
			uint globalY(){ return y; }

			static uint typeId;
		};

	}
}