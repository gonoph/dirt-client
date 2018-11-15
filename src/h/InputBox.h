#ifndef DIRT_INPUTBOX_H
#define DIRT_INPUTBOX_H

#include "Window.h"
#include "InputLine.h"

class InputBox : public Window {
 public:

    InputBox (Window *_parent, char *_prompt, history_id id);

    NAME(InputBox);

    virtual void execute(const char *) = 0;

 private:
    void *extra_data;
    char *prompt;
    InputLine * input;

    void handle_input (char *s);

    virtual bool keypress(int key);
    virtual void redraw();

    virtual bool canCancel() { return true; };
};

#endif

