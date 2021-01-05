#include "SLI.h" 
#include <M5EPD.h>
#include <ArduinoJson.h>
#include "GUI.h"
/**
 * Constructor
 ***/
SliderPage::SliderPage(JsonObject obj, GUI_pos_t cp)
{
    canvas_pos=cp;
    image1 = obj["image1"].as<String>();
    image2 = obj["image2"].as<String>();
    itemId = obj["id"].as<String>();
    header = obj["head"].as<String>();
    factor = obj["factor"].as<float>();
    /**
     * Check if images are used available, if not they will be downloaded
     **/
    GUI_CheckImage(image1);
    GUI_CheckImage(image2);
    canvas.createCanvas(320,240);
}

/**
 * Activate Page
 ***/
void SliderPage::activate()
{
    active = true;
    canvas.fillCanvas(BLACK);
    Serial.printf("SLI INF Drawing Side and imgs\r\n");
    canvas.drawFastVLine(SLIDER_X, SLIDER_Y, SLIDER_HEIGHT, WHITE);
    canvas.drawFastVLine(SLIDER_X + SLIDER_WIDTH, SLIDER_Y, SLIDER_HEIGHT, WHITE);
    canvas.drawBmpFile(SD, image1.c_str(), IMG_POS1_X, IMG_POS1_Y);
    canvas.drawBmpFile(SD, image2.c_str(), IMG_POS2_X, IMG_POS2_Y);
    Serial.printf("SLI INF Update Slider n\r\n");
    GUI_pos_t pos;
    pos.y=-1;
    pos.x=-1;
    handleInput(pos);
    renderHeader(header.c_str());
    canvas.drawRect(0, 0, PAGE_WIDTH, PAGE_HEIGHT , 15);
    canvas.pushCanvas(canvas_pos.x,canvas_pos.y,UPDATE_MODE_GC16);
    draw();
}

/**
 * Header Render function
 * */
void SliderPage::renderHeader(const char *string)
{
    canvas.setTextSize(1);
    canvas.fillRect(3, 4, 316, 20, 15);
    canvas.setTextColor(0);
    canvas.setTextDatum(TC_DATUM);
    canvas.drawString(string, 160, 3, 4);
}

/**
 * Deactivate Page
 ***/
void SliderPage::deActivate()
{
    active = false;
}

/***
 * Return Page header
 **/
String SliderPage::getHeader()
{
    return (header);
}
/**
 * Draw active part (horizontal lines of slider)
 **/
void SliderPage::draw()
{
    uint32_t z = 0;
    /**
     * First create the lower/active part
     **/
    for (z = 0; z < state; z++)
    {
        uint16_t color = 0;
        if (state > 189)
        {
            color = WHITE;
        }
        color = ((state / 6) << 11) | ((state / 3) << 5 | (state / 6));
        canvas.drawFastHLine(SLIDER_X + 5, (SLIDER_Y + 200) - (z * 2), (SLIDER_WIDTH - 10), 15);
        canvas.drawFastHLine(SLIDER_X + 5, (SLIDER_Y + 200) - (z * 2 + 1), (SLIDER_WIDTH - 10), 15);
    }
    /**
     * Fill the rest black
     **/
    for (z = state; z < 100; z++)
    {
        canvas.drawFastHLine(SLIDER_X + 5, (SLIDER_Y + 200) - (z * 2), (SLIDER_WIDTH - 10), BLACK);
        canvas.drawFastHLine(SLIDER_X + 5, (SLIDER_Y + 200) - (z * 2 + 1), (SLIDER_WIDTH - 10), BLACK);
    }
    
    canvas.pushCanvas(canvas_pos.x,canvas_pos.y,UPDATE_MODE_DU4);
    Serial.printf("SLI INF  Draw Slider \r\n");
}
/**
 * Is called when middle button is pushed when page is active
 ****/
void SliderPage::middleButtonPushed()
{
    JsonRPC::execute_boolean("RequestAction", itemId + ",0");
    state = 0;
    draw();
}
/**
 * Loop function, called when active
 ***/
M5EPD_Canvas * SliderPage::handleInput(GUI_pos_t posT)
{
    int32_t pos=getSliderPos(posT);
    /**
     * Just to be sure if we are active
     ***/
    if (active)
    {
        /***
         * Function checks if touc is active and slider is changed by touch
         **/
        /**
         * if value >=0 slider was changed*/
        if (pos >= 0)
        {
            /**
             * Store current position
             * and update slider rendering
             * set flag that slider is currently changed
             * */
            state = pos;
            draw();
            sliderActive = true;
            /**
             * If 500 ms passed, send the information to symcon
             **/
            if ((millis() - lastUpdate) > 500)
            {
                /**
                 * Scale value by factor and send it to server via requestAction
                 * Reset timer for update intervall
                 * TODO: Add option for variables only
                ***/
                int val = (int)(factor * (float)state);
                JsonRPC::execute_boolean("RequestAction", itemId + "," + val);
                sliderLastSendValue = state;
                lastUpdate = millis();
            }
        }
        else
        {
            /***
            * If slider was active but is now not active anymoew
            * Scale value by factor and send it to server via requestAction
            * Reset timer for update intervall
            * TODO: Add option for variables only
            * **/
            if (sliderActive)
            {
                int val = (int)(factor * (float)state);
                JsonRPC::execute_boolean("RequestAction", itemId + "," + val);
                sliderLastSendValue = -11;
                state = -11;
                sliderActive = false;
                lastUpdate = millis();
            }
            else /** No slider activity, if update time passed gte value from server**/
            {
                if ((millis() - lastUpdate) > 500)
                {
                    lastUpdate = millis();
                    uint32_t uVal = JsonRPC::execute_int("GetValue", String(itemId));
                    if (JsonRPC::checkStatus())
                    {
                        int32_t sVal = (int32_t)(((float)(uVal)) / factor);
                        if (sVal != state)
                        {
                            Serial.printf("SLI INF Slider Value changed by server %d", sVal);
                            state = sVal;
                            draw();
                        }
                    }
                }
            }
        }
    }
    return(&canvas);
}

/**
 * Function returns -1 in case of no touch event to slider,
 * or updated slider value from touch event
 **/
int32_t SliderPage::getSliderPos(GUI_pos_t pos)
{
    int retVal = -1;
    if ((pos.x > SLIDER_X) && (pos.x < SLIDER_X + SLIDER_WIDTH))
    {
        if ((pos.y > SLIDER_Y) && (pos.y < (SLIDER_Y + SLIDER_HEIGHT)))
        {
            retVal = (SLIDER_HEIGHT - (pos.y - SLIDER_Y)) / 2;
        }
    }
    return (retVal);
}
