#include "stdafx.h"
#include "event/events.h"


uint Gears::Event::MouseMove::typeId(WM_MOUSEMOVE);
uint Gears::Event::KeyPressed::typeId(WM_KEYDOWN);
uint Gears::Event::KeyReleased::typeId(WM_KEYUP);
uint Gears::Event::MousePressedLeft::typeId(WM_LBUTTONDOWN);
uint Gears::Event::MouseReleasedLeft::typeId(WM_LBUTTONUP);
uint Gears::Event::MousePressedMiddle::typeId(WM_MBUTTONDOWN);
uint Gears::Event::MouseReleasedMiddle::typeId(WM_MBUTTONUP);
uint Gears::Event::MousePressedRight::typeId(WM_RBUTTONDOWN);
uint Gears::Event::MouseReleasedRight::typeId(WM_RBUTTONUP);
uint Gears::Event::Wheel::typeId(WM_MOUSEWHEEL);
uint Gears::Event::StimulusStart::typeId(WM_USER);
uint Gears::Event::Frame::typeId(WM_USER+1);
uint Gears::Event::StimulusEnd::typeId(WM_USER+2);

