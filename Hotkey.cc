#include "dirt.h"
#include "cui.h"
#include "Hotkey.h"
#include "InputBox.h"
#include "ScrollbackSearch.h"
#include "Interpreter.h"
#include "Alias.h"
#include "Session.h"
#include "Chat.h"

Hotkey *hotkey;

bool Hotkey::keypress (int key)
{
    switch (key)
    {
    case key_alt_a:
        (void) new AliasSelection(screen, currentSession ? &currentSession->mud : &globalMUD, "Aliases");
        break;

    case key_alt_i:
        (void) new ActionSelection(screen, "Actions");
        break;

    case key_alt_m:
        (void) new MacroSelection(screen, "Macros");
        break;
        
    case key_alt_q: 		// Quit
        dirtFinished = true;
        break;
        
    case key_alt_r:
        interpreter.dirtCommand("reopen");
        break;
        
    case key_alt_v:
        outputWindow->printVersion();
        break;
        
    case key_alt_s:
        if (!currentSession)
            status->setf ("No active session");
        else
            currentSession->show_nsw();
        break;
        
    case key_alt_o:
        (void) new MUDSelection(screen);
        break;
        
    case key_ctrl_t:
        if (!currentSession)
            status->setf ("No active session");
        else
            currentSession->show_timer();
        break;
        
//    case key_alt_c:  // FIXME doesn't work?
//        interpreter.dirtCommand("close");
//        break;

    case key_alt_c:
        if (!chatServerSocket)
            status->setf("CHAT is not active. Set chat.name option in the config file");
        else
            (void) new ChatWindow(screen);
        break;
        
    case key_alt_slash:
        (void) new ScrollbackSearch(false);
        break;
        
        // Enter scrollback					
    case key_page_up:
    case key_home:
    case key_pause:
        outputWindow->getsb()->keypress(key);
//        (new ScrollbackController(screen, outputWindow))->keypress(key);
        break;
        
    default:
        return false;
    }
    
    return true;
}
