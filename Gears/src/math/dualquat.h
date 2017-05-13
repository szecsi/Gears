#pragma once

#include <math.h>

namespace Gears { namespace Math {

class dualquat
{
public:
	float4 q;
	float4 t;

	dualquat()
		:q(float4::identity),t(float4::zero)
	{
	}

	dualquat(float4 q, float3 t)
	{
		this->q = q;
		this->t[0] = 0.5f*( t[0]*q[0] + t[1]*q[3] - t[2]*q[2]);
		this->t[1] = 0.5f*(-t[0]*q[3] + t[1]*q[0] + t[2]*q[1]);
		this->t[2] = 0.5f*( t[0]*q[2] - t[1]*q[1] + t[2]*q[0]);
		this->t[3] = -0.5f*(t[0]*q[1] + t[1]*q[2] + t[2]*q[3]);
	}

	dualquat(float4 q, float4 t)
		:q(q),t(t)
	{
	}

	dualquat operator*(const dualquat& o) const
	{
		return dualquat(
			q.quatMul(o.q),
			q.quatMul(o.t)+t.quatMul(o.q));
	}

	static const dualquat identity;
};

}} // namespace Gears::Math
