// Output window implementation

#include "OutputWindow.h"
#include "InputBox.h"
#include "ScrollbackSearch.h"
#include "StatusLine.h"
#include "Config.h"
#include "Color.h"
#include "TTY.h"
#include "Hook.h"
#include "misc.h"

#include <cerrno>
#if __GNUC__ >= 3
#include <locale>       // isspace
#else
#include <ctype.h>
#endif

OutputWindow::OutputWindow(Window *_parent)
: Window(_parent, wh_full, _parent->height-1), top_line(0)
{
    // Forget the canvas
    delete[] canvas;

    highlight.line = -1;
    
    scrollback = new attrib[width * config->getOption(opt_scrollback_lines)];
    viewpoint = canvas = scrollback; // point at the beginning
    fFrozen = false;
    clear();
    screen->setScrollingRegion(trueX(), trueY(), trueX() + width, trueY() + height);
    sb = new ScrollbackController(screen, this); // FIXME
}

OutputWindow::~OutputWindow()
{
    canvas = NULL; // Make sure noone delets this
    delete scrollback;
    delete sb;
}

#define COPY_LINES 250	

// 'scroll' one line up
bool OutputWindow::scroll()
{
    if (canvas == scrollback + width * (config->getOption(opt_scrollback_lines) - height))
    {
        // We're at the end of our buffer. Copy some lines up
        memmove(scrollback, scrollback + width * COPY_LINES, 
                (config->getOption(opt_scrollback_lines) - COPY_LINES) * width * sizeof(attrib));
        canvas -= width * COPY_LINES;
        viewpoint -= width * COPY_LINES;

        top_line += COPY_LINES;

        if (viewpoint < scrollback)
            viewpoint = scrollback;
        
        // OK, now clear one full screen beneath
        canvas += width * height; // Cheat :)
        clear();
        canvas -= width * height;
    }
    else
    {
        canvas += width; // advance the canvas
        clear_line(height-1); // Make sure the bottom line is cleared
        cursor_y--;

        if (!fFrozen) // Scroll what we are viewing, too
            viewpoint += width;
    }
    return true;
}

void OutputWindow::moveViewpoint(int amount)
{
	bool fQuit = false;
	
	if (amount < 0 && viewpoint == scrollback)	
		status->setf ("You are already at the beginning of the scrollback buffer");
	else if (amount > 0 && viewpoint == canvas)
		fQuit = true; // Leave
	else
	{
            hook.enableGroup("ScrollbackController_volatile");
            viewpoint += amount * width;
            if (viewpoint < scrollback) {  // Don't scroll past top
                    viewpoint = scrollback;
            }
            if (viewpoint > canvas) { // We're at the end of the scrollback buffer.
                    viewpoint = canvas;
            }
            if(viewpoint == canvas) fQuit = true;
            status->sticky_status = true;
            status->setf("Scrollback: line %d of %d",
                    (viewpoint - scrollback) / width,
                    (canvas-scrollback) / width + height);
	}
	dirty = true;
        if(fQuit) sb->close();
}

ScrollbackController::ScrollbackController(Window *_parent, OutputWindow *_output) 
    : Window(_parent,0,0,None), output(_output) {
    vector<string> mygroups;
    mygroups.push_back("Dirt keys");
    mygroups.push_back("ScrollbackController");
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true,
        "__DIRT_ScrollbackController_page_up", mygroups, "", "", 
        key_page_up, "", &ScrollbackController::keypress_page_up, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_ScrollbackController_page_down", mygroups, "", "", 
        key_page_down, "", &ScrollbackController::keypress_page_down, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_ScrollbackController_home", mygroups, "", "", 
        key_home, "", &ScrollbackController::keypress_home, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_ScrollbackController_pause", mygroups, "", "", 
        key_pause, "", &ScrollbackController::keypress_pause, (void*)this));
    mygroups.push_back("ScrollbackController_volatile");
    hook.add(KEYPRESS, new KeypressHookStub(1, 1.0, -1, false, true, true, 
        "__DIRT_ScrollbackController_end", mygroups, "", "", 
        key_end, "", &ScrollbackController::keypress_end, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(1, 1.0, -1, false, false, true, 
        "__DIRT_ScrollbackController_arrow_up", mygroups, "", "", 
        key_arrow_up, "", &ScrollbackController::keypress_arrow_up, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(1, 1.0, -1, false, false, true, 
        "__DIRT_ScrollbackController_arrow_down", mygroups, "", "", 
        key_arrow_down, "", &ScrollbackController::keypress_arrow_down, (void*)this));
}

bool ScrollbackController::keypress_page_up(string&, void* mt) {
    ScrollbackController* mythis = (ScrollbackController*)mt;
    mythis->output->moveViewpoint(-mythis->output->height/2);
    mythis->output->freeze();
    return true;
}

bool ScrollbackController::keypress_page_down(string&, void* mt) {
    ScrollbackController* mythis = (ScrollbackController*)mt;
    mythis->output->moveViewpoint(mythis->output->height/2);
    return true;
}

// FIXME doesn't do anything.
bool ScrollbackController::keypress_arrow_up(string&, void* mt) {
    ScrollbackController* mythis = (ScrollbackController*)mt;
    mythis->output->moveViewpoint(-1);
    return true;
}

// FIXME doesn't do anything.
bool ScrollbackController::keypress_arrow_down(string&, void* mt) {
    ScrollbackController* mythis = (ScrollbackController*)mt;
    mythis->output->moveViewpoint(1);
    return true;
}

// FIXME doesn't do anything.
bool ScrollbackController::keypress_home(string&, void* mt) {
    ScrollbackController* mythis = (ScrollbackController*)mt;
    mythis->output->moveViewpoint(-(mythis->output->viewpoint-mythis->output->scrollback)/mythis->output->width);
    return true;
}

// FIXME What is this supposed to do?
bool ScrollbackController::keypress_pause(string&, void* mt) {
    ScrollbackController* mythis = (ScrollbackController*)mt;
    mythis->output->moveViewpoint(0);
    return true;
}

// FIXME doesn't do anything.
bool ScrollbackController::keypress_end(string&, void* mt) {
    ((ScrollbackController*)mt)->close();
    return true;
}

// FIXME disable keys here.
void ScrollbackController::close() {
    hook.disableGroup("ScrollbackController_volatile"); // Disable my keys.
    status->sticky_status = false;
    status->setf ("Leaving scrollback.");
    output->unfreeze();
    show(false);
}

void OutputWindow::search (const char *s, bool forward)
{
    attrib *p = viewpoint + (width * (cursor_y-1)), *q, *r;
    const char *t;
    int len = strlen(s);
    bool found = true;
    
    for (;;)
    {
        // Search current line first
        // Start search at beginning and continue until <len> offset from the right margin
        for (q = p; q < p + width - len; q++)
        {
            found = true;
            // Now compare the letters
            for (r = q, t = s; *t; t++, r++)
            {
                if ( tolower((*r & 0xFF)) != tolower(*t))
                {
                    found = false;
                    break;
                }
            }

            if (found)
                break;

        }
        
        if (found)
            break;
        
        // Go forward or backward
        if (forward)
        {
            if (p == canvas + width * cursor_y) // we're searching the very last line!
                break;
            
            p += width;
        }
        else
        {
            if (p == scrollback)
                break;
            
            p -= width;
        }
    }
    
    if (!found)
        status->setf ("Search string '%s' not found", s);
    else
    {
        highlight.line = ((p-scrollback) / width) + top_line;
        highlight.x = q - p;
        highlight.len = strlen(s);
        
        // Show on the second line rather than under status bar
        // FIXME
        //viewpoint = scrollback >? p-width;
        viewpoint = (scrollback > p-width)?scrollback:(p-width);
        viewpoint = (viewpoint < canvas)?viewpoint:canvas;
        //viewpoint = viewpoint <? canvas;
        status->setf("Found string '%s'", s);
    }
}


void OutputWindow::draw_on_parent()
{
    // Show what we are viewing (not necessarily the same as what we are drawing on
    if (parent)
    {
        
        // Now we need to highlight search strings, if any
        if (highlight.line >= 0 &&
            highlight.line-top_line >= (viewpoint-scrollback) / width &&
            highlight.line-top_line <= ((viewpoint-scrollback) / width + height))
        {
            attrib *a, *b;
            a = viewpoint + width * ( (highlight.line - top_line) - (viewpoint-scrollback)/width) + highlight.x;

            // Copy the old stuff into a temp array ,ugh
            // This seems to be the easiest solution however
            attrib old[highlight.len];
            memcpy(old, a, highlight.len*sizeof(attrib));
            for (b = a; b < a+highlight.len; b++)
            {
                unsigned char color = ((*b & 0xFF00) >> 8) & ~(fg_bold|fg_blink);
                unsigned char bg = (color & 0x0F) << 4;
                unsigned char fg = (color & 0xF0) >> 4;

                *b = (*b & 0x00FF) | ((bg|fg) << 8);
            }

            parent->copy (viewpoint, width,height,parent_x,parent_y);

            memcpy(a, old, highlight.len*sizeof(attrib));
            
        }
        else
            parent->copy (viewpoint, width,height,parent_x,parent_y);
        
    }
}

void OutputWindow::move (int x, int y) {
    Window::move(x,y);
    screen->setScrollingRegion(trueX(), trueY(), trueX() + width, trueY() + height);
}

// Print some version information
void OutputWindow::printVersion()
{
    printf (
            "\n"
            PACKAGE_NAME " version " VERSION "\n"
            COPYRIGHT "\n"
            PACKAGE_NAME " is based on priot code; for details, see AUTHORS\n"
            PACKAGE_NAME " comes with ABSOLUTELY NO WARRANTY; for details, see file COPYING\n"
            "This binary compiled: " __DATE__ ", " __TIME__ "\n"
            "Binary/source available from %s\n",
            PACKAGE_URL
           );

    if (screen->isVirtual())
        printf("Using Virtual Console output routines\n");
    else
        printf("Using TTY output routines\n");
}

void OutputWindow::saveToFile (const char *fname, bool use_color) {
    FILE *fp = fopen(fname, "w");
    if (!fp)
        status->setf("Cannot open %s for writing: %s", fname, strerror(errno));
    else {
        fprintf(fp, "Scrollback saved from dirt %s at %s", VERSION, ctime(&current_time));

        int color = -1;
        for (attrib *line = scrollback; line < canvas + (width * height); line += width) {
            for (attrib *a = line; a < line+width; a++) {
                if (use_color && (*a >> 8) != color) {
                    fputs(screen->getColorCode(*a >> 8, true), fp);
                    color = *a >> 8;
                }
                fputc(*a & 0xFF, fp);
            }
            fputc('\n', fp);
        }
        fclose(fp);
        status->setf("Scrollback saved successfully");
    }
}

void ScrollbackSearch::execute(const char *text)
{
    ScrollbackController *s;
    
    if (text[0]) {
        // If the scrollback is not up yet, we need to create it
        if (!( s = (ScrollbackController*) (screen->findByName ("ScrollbackController"))))
            (s = new ScrollbackController(screen, outputWindow))->keypress(key_pause); // FIXME
        
        outputWindow->search(text, forward);
    }

    die();
}

ScrollbackController::~ScrollbackController() {
    hook.remove("__DIRT_ScrollbackController_page_up");
    hook.remove("__DIRT_ScrollbackController_page_down");
    hook.remove("__DIRT_ScrollbackController_arrow_up");
    hook.remove("__DIRT_ScrollbackController_arrow_down");
    hook.remove("__DIRT_ScrollbackController_home");
    hook.remove("__DIRT_ScrollbackController_pause");
    hook.remove("__DIRT_ScrollbackController_end");
}
