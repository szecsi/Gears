#include "stdafx.h"
#include "PythonDict.h"

#include "Response.h"
#include "Sequence.h"
#include "SpatialFilter.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <limits>

#include "Eigen/Dense"
#include "Eigen/SVD"


Response::Response():
	question("Pizza?"),
	loop(false),
	duration(1),
	startingFrame(0)
{
}


boost::python::object Response::setPythonObject(boost::python::object o)
{
	pythonObject = o;
	return boost::python::object();
}

boost::python::object Response::getPythonObject () const
{
	return pythonObject;
}


boost::python::object Response::setJoiner(boost::python::object joiner)
{
	this->joiner = joiner;
	return boost::python::object();
}

void Response::registerCallback(uint msg, boost::python::object callback)
{
	for (auto& o : callbacks[msg])
		if (o == callback)
			return;
	callbacks[msg].push_back(callback);
}


void Response::addButton(std::string label, float x, float y, float w, float h, uint key, bool visible)
{
	Button b = {label, x, y, w, h, key, visible};
	buttons.push_back(b);
}