// InputBox
// Simple dialog box which prompts you about something
// It centers itself on its parent

#include "dirt.h"
#include "InputBox.h"

class InputBoxedLine : public InputLine
{
public:
    InputBoxedLine(Window *_parent, int _w, int _h, int _x, int _y, history_id _id)
        : InputLine (_parent, _w, _h, None, _x, _y, _id)
    {
    };

private:
    virtual void execute() {
        ((InputBox*)parent)->execute(input_buf);
    }
};

InputBox::InputBox (Window *_parent, char *_prompt, history_id id)
: Window(_parent, strlen(_prompt)+4, 7, Bordered, xy_center,xy_center ),
  prompt(strdup(_prompt)) {
	input = new InputBoxedLine(this,
		width-2,1, 					// 2 chars smaller than the inside
		1,3,						// 1 char from left, in the middle
		id);

	input->set_prompt("");
}

void InputBox::redraw() {
	set_color (bg_blue|fg_white);
	clear();
	gotoxy(1,1);
	print (prompt);
	dirty = false;
}

bool InputBox::keypress(int key) {
    if (key == key_escape)     {
        if (canCancel())
            die();
        return true;
    }
    else
        return Window::keypress(key);
}
