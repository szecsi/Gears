#pragma once

// dependencies
#include <vector>
#include "memory"
#include <boost/python.hpp>

// poly2tri headers
#include "poly2tri.h"

using namespace boost::python;

namespace p2t
{
	class Poly2TriWrapper
	{
		std::unique_ptr<CDT> instance;
		std::vector<Point*> points;
		std::vector<Point> points_store;
	public:
		Poly2TriWrapper(const boost::python::list& pts);
		boost::python::list GetTriangles();
		void Triangulate();
	};
}