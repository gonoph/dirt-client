// Numbered windows automatically number themselves with the first free
// number

class Numbered
{
public:
    int number; // -1 if no number available
    Window *window;
    
    Numbered(Window *w) : window(w)
    {
        int i;

        for (i = 1; i < 10; i++)
            if (!list[i])
                break;

        if (i == 10)
            number = -1;
        else
        {
            list[i]  = w;
            number = i;
        }

    }

    ~Numbered()
    {
        if (number != -1)
            list[number] = NULL;
    }

    static Window* find (int i)
    {
        return list[i];
    }

    static Window* list[10];
};


