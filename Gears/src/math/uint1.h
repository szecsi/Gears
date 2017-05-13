#pragma once

#include <math.h>
#include <stdexcept>

namespace Gears { namespace Math {

class uint2;
class uint3;
class uint4;

class uint1
{
public:
	union{
		struct {
			uint x;
		};

		uint v[1];

		intswizzle<1, uint2, bool2, 0, 0> xx;
		intswizzle<1, uint3, bool3, 0, 0, 0> xxx;
		intswizzle<1, uint4, bool4, 0, 0, 0, 0> xxxx;

	};

	operator uint() const
	{
		return x;
	}

	uint1():x(0){}

	uint1(uint i):x(i){}

	uint1(uint x, uint y, uint z, uint w):x(x){y; z; w;}

	uint1(bool1 b):x(b.x){}

	uint1& operator+=(const uint1& o)
	{
		x += o.x;
		return *this;
	}

	uint1& operator-=(const uint1& o)
	{
		x -= o.x;
		return *this;
	}

	uint1& operator*=(const uint1& o)
	{
		x *= o.x;
		return *this;
	}

	uint1& operator/=(const uint1& o)
	{
		x /= o.x;
		return *this;
	}

	uint1& operator%=(const uint1& o)
	{
		x %= o.x;
		return *this;
	}

	uint1& operator>>=(const uint1& o)
	{
		x >>= o.x;
		return *this;
	}

	uint1& operator<<=(const uint1& o)
	{
		x <<= o.x;
		return *this;
	}

	uint1& operator&=(const uint1& o)
	{
		x &= o.x;
		return *this;
	}

	uint1& operator|=(const uint1& o)
	{
		x |= o.x;
		return *this;
	}

	uint1 operator&(const uint1& o) const
	{
		return uint1(x & o.x);
	}

	uint1 operator&&(const uint1& o) const
	{
		return uint1(x && o.x);
	}

	uint1 operator|	(const uint1& o) const
	{
		return uint1(x | o.x);
	}

	uint1 operator||(const uint1& o) const
	{
		return uint1(x || o.x);
	}

	bool2 operator==(const uint1& o) const
	{
		return bool2(x == o.x);
	}

	bool2 operator!=(const uint1& o) const
	{
		return bool2(x != o.x);
	}

	bool2 operator<(const uint1& o) const
	{
		return bool2(x < o.x);
	}

	bool2 operator>(const uint1& o) const
	{
		return bool2(x > o.x);
	}

	bool2 operator<=(const uint1& o) const
	{
		return bool2(x <= o.x);
	}

	bool2 operator>=(const uint1& o) const
	{
		return bool2(x >= o.x);
	}

	uint1 operator<<(const uint1& o) const
	{
		return uint1(x << o.x);
	}

	uint1 operator>>(const uint1& o) const
	{
		return uint1(x >> o.x);
	}

	uint1 operator+(const uint1& o) const
	{
		return uint1(x + o.x);
	}

	uint1 operator-(const uint1& o) const
	{
		return uint1(x - o.x);
	}

	uint1 operator*(const uint1& o) const
	{
		return uint1(x * o.x);
	}

	uint1 operator/(const uint1& o) const
	{
		return uint1(x / o.x);
	}

	uint1 operator%(const uint1& o) const
	{
		return uint1(x % o.x);
	}

	uint1 operator+() const
	{
		return uint1(+x);
	}

	uint1 operator!() const
	{
		return uint1(!x);
	}

	uint1 operator~() const
	{
		return uint1(~x);
	}

	uint1 operator++()
	{
		return uint1(++x);
	}

	uint1 operator--()
	{
		return uint1(--x);
	}


	uint1 operator++(int)
	{
		return uint1(x++);
	}
	
	uint1 operator--(int)
	{
		return uint1(x++);
	}


	uint operator[](uint i) const
	{
		if(i < 0 || i > 0)
			throw std::range_error("uint1 index out of range.");
		return v[i];
	}

	uint& operator[](uint i)
	{
		if(i < 0 || i > 0)
			throw std::range_error("uint1 index out of range.");
		return v[i];
	}

	uint1 max(const uint1& o) const
	{
		return uint1( (x>o.x)?x:o.x );
	}

	uint1 min(const uint1& o) const
	{
		return uint1( (x<o.x)?x:o.x );
	}

	static uint1 random(uint lower=0, uint upper=6)
	{
		uint range = upper - lower + 1;
		return uint1(
			rand() % range + lower);
	}

	static const uint1 zero;
	static const uint1 xUnit;
	static const uint1 one;

};

}} // namespace Gears::Math
