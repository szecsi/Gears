#pragma once

#include <algorithm>
#include <string>
#include <map>
#include "math/math.h"
#include <boost/parameter/keyword.hpp>
#include <boost/parameter/preprocessor.hpp>
#include <boost/parameter/python.hpp>
#include <boost/python.hpp>
#include <iomanip>
#include <list>
#include <set>
#include <vector>
#include "Pass.h"
#include "event/Base.h"

class Sequence;
class SpatialFilter;

//! A structure that contains all stimulus parameters.
class Response
{

public:
	std::string question;
	bool loop;

	struct Button{
		std::string label;
		float xcoord, ycoord, width, height;
		uint key;
		bool visible;
	};
	std::vector<Button> buttons;
	int duration; //frames
	int startingFrame;

	boost::shared_ptr<Sequence> sequence;		//< Part of this sequence.

	
	Response();
	GEARS_SHARED_CREATE_WITH_GETSHAREDPTR(Response);

	boost::python::object pythonObject;
	boost::python::object setPythonObject(boost::python::object o);
	boost::python::object getPythonObject () const;

	boost::python::object joiner;
	boost::python::object setJoiner(boost::python::object joiner);


	std::map<uint, std::vector<boost::python::object> > callbacks;
	void registerCallback(uint msg, boost::python::object callback);
	template<typename T>
	bool executeCallbacks(typename T::P wevent) const
	{
		bool handled = false;
		auto ic = callbacks.find(T::typeId);
		if (ic != callbacks.end())
			for (auto& cb : ic->second)
			{
				boost::python::object rhandled = cb(wevent);
				boost::python::extract<bool> exb(rhandled);
				if (exb.check())
					handled = handled || exb();
			}
		return handled;
	}
	
	void addButton(std::string label, float x, float y, float w, float h, uint key, bool visible);

	void setSequence(boost::shared_ptr<Sequence> sequence)
	{
		this->sequence = sequence;
	}

	boost::shared_ptr<Sequence> getSequence() { return sequence; }


};
