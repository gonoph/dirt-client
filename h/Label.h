// A label is just a window displaying a text


class Label : public Window
{
public:
    Label(Window *_parent, const char *_text, int _width, int _height, int _x, int _y)
        : Window(_parent, _width ?: strlen(_text), _height, None, _x, _y), text(_text)
    {
        set_color(bg_blue|fg_white);
        clear();
    }
    
    void redraw() { gotoxy(0,0); print(text);}

private:
    String text;
};

