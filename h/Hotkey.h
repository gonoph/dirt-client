#include "Control.h"
class Hotkey : public Control {
public:
    Hotkey(Window *_parent) : Control(_parent) {}
    virtual bool  keypress (int key);
    NAME(Hotkey);
};

extern bool dirtRestart;
