#pragma once
#include <M5EPD.h>
#include "GUI.h"
/**
 * Abstract Class for GUI Page
 * Areas for M5Core2 Size is 320x215
 * **/
class Page
{
    protected:
    bool active=false;
    GUI_pos_t canvas_pos;
    public:
    virtual void activate();
    virtual void deActivate();
    virtual void draw();
    virtual M5EPD_Canvas * handleInput(GUI_pos_t pos);
    virtual void middleButtonPushed();
    virtual String getHeader();
   
         
};