#ifndef DIRT_CONTROL_H
#define DIRT_CONTROL_H

#include "Window.h"

// A control is an invisible window, probably catching keypresses
class Control : public Window {
 public:
    Control(Window *_parent) : Window (_parent,0,0)
    {
	show(false);
    };
};

#endif
