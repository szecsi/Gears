#pragma once

#include <math.h>
#include <stdexcept>

namespace Gears { namespace Math {

class uint3;
class uint4;

class uint2
{
public:
	union{
		struct {
			uint x;
			uint y;
		};

		uint v[2];

		intswizzle<2, uint2, bool2, 0, 0> xx;
		intswizzle<2, uint2, bool2, 0, 1> xy;
		intswizzle<2, uint2, bool2, 1, 0> yx;
		intswizzle<2, uint2, bool2, 1, 1> yy;

		intswizzle<2, uint3, bool3, 0, 0, 0> xxx;
		intswizzle<2, uint3, bool3, 0, 0, 1> xxy;
		intswizzle<2, uint3, bool3, 0, 1, 0> xyx;
		intswizzle<2, uint3, bool3, 0, 1, 1> xyy;
		intswizzle<2, uint3, bool3, 1, 0, 0> yxx;
		intswizzle<2, uint3, bool3, 1, 0, 1> yxy;
		intswizzle<2, uint3, bool3, 1, 1, 0> yyx;
		intswizzle<2, uint3, bool3, 1, 1, 1> yyy;

		intswizzle<2, uint4, bool4, 0, 0, 0, 0> xxxx;
		intswizzle<2, uint4, bool4, 0, 0, 0, 1> xxxy;
		intswizzle<2, uint4, bool4, 0, 0, 1, 0> xxyx;
		intswizzle<2, uint4, bool4, 0, 0, 1, 1> xxyy;
		intswizzle<2, uint4, bool4, 0, 1, 0, 0> xyxx;
		intswizzle<2, uint4, bool4, 0, 1, 0, 1> xyxy;
		intswizzle<2, uint4, bool4, 0, 1, 1, 0> xyyx;
		intswizzle<2, uint4, bool4, 0, 1, 1, 1> xyyy;
		intswizzle<2, uint4, bool4, 1, 0, 0, 0> yxxx;
		intswizzle<2, uint4, bool4, 1, 0, 0, 1> yxxy;
		intswizzle<2, uint4, bool4, 1, 0, 1, 0> yxyx;
		intswizzle<2, uint4, bool4, 1, 0, 1, 1> yxyy;
		intswizzle<2, uint4, bool4, 1, 1, 0, 0> yyxx;
		intswizzle<2, uint4, bool4, 1, 1, 0, 1> yyxy;
		intswizzle<2, uint4, bool4, 1, 1, 1, 0> yyyx;
		intswizzle<2, uint4, bool4, 1, 1, 1, 1> yyyy;

	};

	uint2():x(0),y(0){}

	uint2(uint i):x(i),y(i){}

	uint2(uint x, uint y):x(x),y(y){}

	uint2(uint x, uint y, uint z, uint w):x(x),y(y){z; w;}

	uint2(bool2 b):x(b.x),y(b.y){}

	uint2& operator+=(const uint2& o)
	{
		x += o.x;
		y += o.y;
		return *this;
	}

	uint2& operator-=(const uint2& o)
	{
		x -= o.x;
		y -= o.y;
		return *this;
	}

	uint2& operator*=(const uint2& o)
	{
		x *= o.x;
		y *= o.y;
		return *this;
	}

	uint2& operator/=(const uint2& o)
	{
		x /= o.x;
		y /= o.y;
		return *this;
	}

	uint2& operator%=(const uint2& o)
	{
		x %= o.x;
		y %= o.y;
		return *this;
	}

	uint2& operator>>=(const uint2& o)
	{
		x >>= o.x;
		y >>= o.y;
		return *this;
	}

	uint2& operator<<=(const uint2& o)
	{
		x <<= o.x;
		y <<= o.y;
		return *this;
	}

	uint2& operator&=(const uint2& o)
	{
		x &= o.x;
		y &= o.y;
		return *this;
	}

	uint2& operator|=(const uint2& o)
	{
		x |= o.x;
		y |= o.y;
		return *this;
	}

	uint2 operator&(const uint2& o) const
	{
		return uint2(x & o.x, y & o.y);
	}

	uint2 operator&&(const uint2& o) const
	{
		return uint2(x && o.x, y && o.y);
	}

	uint2 operator|	(const uint2& o) const
	{
		return uint2(x | o.x, y | o.y);
	}

	uint2 operator||(const uint2& o) const
	{
		return uint2(x || o.x, y || o.y);
	}

	bool2 operator==(const uint2& o) const
	{
		return bool2(x == o.x, y == o.y);
	}

	bool2 operator!=(const uint2& o) const
	{
		return bool2(x != o.x, y != o.y);
	}

	bool2 operator<(const uint2& o) const
	{
		return bool2(x < o.x, y < o.y);
	}

	bool2 operator>(const uint2& o) const
	{
		return bool2(x > o.x, y > o.y);
	}

	bool2 operator<=(const uint2& o) const
	{
		return bool2(x <= o.x, y <= o.y);
	}

	bool2 operator>=(const uint2& o) const
	{
		return bool2(x >= o.x, y >= o.y);
	}

	uint2 operator<<(const uint2& o) const
	{
		return uint2(x << o.x, y << o.y);
	}

	uint2 operator>>(const uint2& o) const
	{
		return uint2(x >> o.x, y >> o.y);
	}

	uint2 operator+(const uint2& o) const
	{
		return uint2(x + o.x, y + o.y);
	}

	uint2 operator-(const uint2& o) const
	{
		return uint2(x - o.x, y - o.y);
	}

	uint2 operator*(const uint2& o) const
	{
		return uint2(x * o.x, y * o.y);
	}

	uint2 operator/(const uint2& o) const
	{
		return uint2(x / o.x, y / o.y);
	}

	uint2 operator%(const uint2& o) const
	{
		return uint2(x % o.x, y % o.y);
	}

	uint2 operator+() const
	{
		return uint2(+x, +y);
	}

	uint2 operator!() const
	{
		return uint2(!x, !y);
	}

	uint2 operator~() const
	{
		return uint2(~x, ~y);
	}

	uint2 operator++()
	{
		return uint2(++x, ++y);
	}

	uint2 operator--()
	{
		return uint2(--x, --y);
	}


	uint2 operator++(int)
	{
		return uint2(x++, y++);
	}
	
	uint2 operator--(int)
	{
		return uint2(x++, y++);
	}


	uint operator[](uint i) const
	{
		if(i < 0 || i > 1)
			throw std::range_error("uint2 index out of range.");
		return v[i];
	}

	uint& operator[](uint i)
	{
		if(i < 0 || i > 1)
			throw std::range_error("uint2 index out of range.");
		return v[i];
	}

	uint2 max(const uint2& o) const
	{
		return uint2( (x>o.x)?x:o.x, (y>o.y)?y:o.y );
	}

	uint2 min(const uint2& o) const
	{
		return uint2( (x<o.x)?x:o.x, (y<o.y)?y:o.y );
	}

	static uint2 random(uint lower=0, uint upper=6)
	{
		uint range = upper - lower + 1;
		return uint2(
			rand() % range + lower,
			rand() % range + lower);
	}

	static const uint2 zero;
	static const uint2 xUnit;
	static const uint2 yUnit;
	static const uint2 one;

};

}} // namespace Gears::Math
