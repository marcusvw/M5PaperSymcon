#include "PAG.h"
#include "RPC.h"
#include "SLI.h"
#include <ArduinoJson.h>
#include "GUI.h"
#include <HTTPClient.h>
#include <M5EPD.h>
#include <Preferences.h>
#include "WIF.h"
#include "BT4.h"
static Page *pages[NUM_PAGES_MAX];
static uint8_t pageCount = 0;
static uint8_t currentPage = 0;
static bool t1 = false;
static bool t2 = false;
static bool t3 = false;
static uint32_t lastActive = 0;
static bool forceDownload = false;
static uint32_t sleepTimeout = SLEEP_TIMEOUT_LONG;
static String imgServer = "";
static uint32_t confVers = 0;
static uint32_t touchReleaseCounter = 0;
static uint32_t notAvailableCounter = 0;
GUI_pos_t dummyPos = {-1, -1};
GUI_pos_t positionTemplate[NUM_PAGES] = {{PAGE_WIDTH * 0, (PAGE_HEIGHT * 0) + OFFSET_Y}, {PAGE_WIDTH * 1, (PAGE_HEIGHT * 0) + OFFSET_Y}, {PAGE_WIDTH * 2, (PAGE_HEIGHT * 0) + OFFSET_Y}, {PAGE_WIDTH * 0, (PAGE_HEIGHT * 1) + OFFSET_Y + PAGE_PADDING_Y}, {PAGE_WIDTH * 1, (PAGE_HEIGHT * 1) + OFFSET_Y + PAGE_PADDING_Y}, {PAGE_WIDTH * 2, (PAGE_HEIGHT * 1) + OFFSET_Y + PAGE_PADDING_Y}};
Preferences GUI__preferences;
GUI_pos_t pos;
void GUI_Init()
{
    String config = JsonRPC::init();
    DynamicJsonDocument doc(20000);
    DeserializationError error = deserializeJson(doc, config);
    for (uint32_t x; x < NUM_PAGES_MAX; x++)
    {
        pages[x] = NULL;
    }
    // Test if parsing succeeds.
    if (error)
    {
        Serial.print(F("GUI ERR deserializeJson() of GUI Config failed: "));
        Serial.println(error.c_str());
        return;
    }
    Serial.println(F("GUI INF JSON Config loaded"));
    /**
     * Get some basic stuff as image server and timeout for sleep from config
     ***/
    imgServer = doc["imagesrv"].as<String>();
    confVers = doc["version"].as<uint32_t>();
    sleepTimeout = doc["sleepTimeout"].as<uint32_t>();
    /**
     * Try to get last config version to check if image files should be re-downloaded
     * **/
    GUI__preferences.begin("GUI", true);
    uint32_t lastVersion = GUI__preferences.getUInt("CONFVER", 0);
    GUI__preferences.end();
    if (confVers != lastVersion)
    {
        Serial.printf("GUI INF New config version found refreshing images old:%d, new:%d\r\n", lastVersion, confVers);
        forceDownload = true;
    }
    /**
     * Iterate through elements in config file
     ***/
    uint32_t elCount = doc["elements"].size();
    Serial.printf("M2I INF Number of Config Elemenets:%d\r\n", elCount);
    uint32_t x;
    uint32_t pageIndex = 0;
    for (x = 0; x < elCount; x++)
    {

        // Check type
        String type = doc["elements"][x]["type"].as<String>();
        Serial.printf("M2I Config Type: %s\r\n", type.c_str());
        // Typpe is slider page
        if (type == "SLI")
        {
            Serial.printf("GUI INF SLI Config: %s %s %s\r\n", doc["elements"][x]["image1"].as<String>().c_str(), doc["elements"][x]["id"].as<String>().c_str(), doc["elements"][x]["factor"].as<String>().c_str());
            pages[pageCount] = new SliderPage((JsonObject)(doc["elements"][x]), positionTemplate[pageIndex]);

            pageCount++;
        }
        // Type is 4 Button Page
        else if (type == "BT4")
        {
            Serial.printf("GUI INF B4T Config: %s \r\n", doc["elements"][x]["head"].as<String>().c_str());
            pages[pageCount] = new Button4Page((JsonObject)(doc["elements"][x]), positionTemplate[pageIndex]);
            pageCount++;
        }
        pageIndex++;
        if (pageIndex > NUM_PAGES)
        {
            pageIndex = 0;
        }
    }
    /**
     * If new version was detected
     * Save the new version number in the nv memory.
     * As the page constructors are downloading the images the force download flag can be restetted
     ****/
    if (forceDownload)
    {

        forceDownload = false;
        GUI__preferences.begin("GUI", false);
        GUI__preferences.putUInt("CONFVER", confVers);
        GUI__preferences.end();
    }
    /***
     * Activate page 0 and render header
     **/

    for (uint32_t x; x < NUM_PAGES; x++)
    {
        if (pages[x] != NULL)
        {
            pages[x]->activate();
        }
    }
    //GUI__header(pages[currentPage]->getHeader().c_str());
    pos.x = -1;
    pos.y = -1;
}

void GUI_Loop()
{
    /**
     * Checks if any button was pushed and changes the selected page (right/left) 
     * or sends the middle button to the page. Returns true if any touch was detected
     * used to reset the sleep timer. The sleeptimer fill overflow after 50 days
     ***/

    if (M5.TP.avaliable())
    {
        bool update = true;
        bool touch = !M5.TP.isFingerUp();
        notAvailableCounter = 0;
        if (!touch)
        {
            if (touchReleaseCounter < 3)
            {
                touchReleaseCounter++;
                touch = true;
                update = false;
            }
        }
        else
        {
            touchReleaseCounter = 0;
        }

        if (touch)
        {
            if (update)
            {
                M5.TP.update();
                tp_finger_t FingerItem = M5.TP.readFinger(0);
                pos.x = FingerItem.x;
                pos.y = FingerItem.y;
            }
            Serial.printf("GUI INF Touch: %d, %d\r\n", pos.x, pos.y);
            if (GUI__checkButtons(pos))
            {
                lastActive = millis();
            }
        }
        else
        {
            pos.x = -1;
            pos.y = -1;
            Serial.printf("GUI INF Touch released\r\n");
            GUI__checkButtons(pos);
        }

        /**
     * Sleep timeout detected-->Goto light sleep and switch off not needed stuff.
     ***/
    }
    else
    {
        if (notAvailableCounter < 8)
        {
            notAvailableCounter++;
            Serial.printf("GUI INF No Data available Keep Touch: %d, %d\r\n", pos.x, pos.y);
        }
        else
        {
            pos.x = -1;
            pos.y = -1;
            //Serial.printf("GUI INF Touch released2\r\n");
            GUI__checkButtons(pos);
        }
    }
    for (int x=0; x < NUM_PAGES; x++)
    {
        if (pages[x] != NULL)
        {

            if (pos.x == -1)
            {

                pages[x]->handleInput(dummyPos);
            }
            else
            {
                if ((pos.x>=positionTemplate[x].x)&&(pos.x<(positionTemplate[x].x+PAGE_WIDTH))&&
                    (pos.y>=positionTemplate[x].y)&&(pos.y<(positionTemplate[x].y+PAGE_HEIGHT)))
                {
                    GUI_pos_t locPos=pos;
                    locPos.x-=positionTemplate[x].x;
                    locPos.y-=positionTemplate[x].y;
                    pages[x]->handleInput(locPos);
                }
                else
                {
                    pages[x]->handleInput(dummyPos);
                }
                
            }
        }
    }
    #if 0
    if (pos.x != -1)
    {
        if (pos.x < 320)
        {
            pages[currentPage]->handleInput(pos);
            pages[currentPage + 1]->handleInput(dummyPos);
            pages[currentPage + 2]->handleInput(dummyPos);
        }
        else if ((pos.x > 320) && (pos.x < 640))
        {
            GUI_pos_t locPos = pos;
            locPos.x -= 320;
            pages[currentPage]->handleInput(dummyPos);
            pages[currentPage + 1]->handleInput(locPos);
            pages[currentPage + 2]->handleInput(dummyPos);
        }
        else if (pos.x > 640)
        {
            GUI_pos_t locPos = pos;
            locPos.x -= 640;
            pages[currentPage]->handleInput(dummyPos);
            pages[currentPage + 1]->handleInput(dummyPos);
            pages[currentPage + 2]->handleInput(locPos);
        }
    }
    else
    {
        pages[currentPage]->handleInput(pos);
        pages[currentPage + 1]->handleInput(pos);
        pages[currentPage + 2]->handleInput(pos);
    }
    #endif
}

/**
 * Write helper function for 1 Wire registers
 ***/
void GUI__WriteNByte(uint8_t addr, uint8_t reg, uint8_t num, uint8_t *data)
{
    Wire1.beginTransmission(addr);
    Wire1.write(reg);
    Wire1.write(data, num);
    Wire1.endTransmission();
}

/**
 * Simple helper function to check if a touch is detected in a specific area
 * **/
bool GUI__isInArea(int xT, int yT, int x, int y, int sizeX, int sizeY)
{
    bool retVal = false;
    if (x != -1)
    {
        retVal = ((xT > 0) && (xT > x) && (xT < (x + sizeX)) && (yT > y) && (yT < (y + sizeY)));
    }
    return (retVal);
}
/**
 * Function to check the state of the 3 virtual touch buttons
 * returns true if any touch is detected.
 * **/
bool GUI__checkButtons(GUI_pos_t pos)
{
    bool touchActive = false;
    touchActive = (pos.x != -1);
    if (touchActive)
    {

        if (GUI__isInArea(pos.x, pos.y, T1_X, T1_Y, SIZE_X, SIZE_Y))
        {
            if (t1 == false)
            {
                t1 = true;
                pages[currentPage]->deActivate();
                pages[currentPage + 1]->deActivate();
                pages[currentPage + 2]->deActivate();
                if (currentPage == 0)
                {
                    currentPage = pageCount - 1;
                }
                else
                {
                    currentPage--;
                }
                Serial.printf("GUI INF T1 Pos x %d, Pos y %d , Page: %d\r\n", pos.x, pos.y, currentPage);
                pages[currentPage]->activate();
                //GUI__header(pages[currentPage]->getHeader().c_str());
            }
        }
        else
        {
            if (t1 == true)
            {
                t1 = false;
            }
        }

        if (GUI__isInArea(pos.x, pos.y, T2_X, T2_Y, SIZE_X, SIZE_Y))
        {
            if (t2 == false)
            {
                t2 = true;
                pages[currentPage]->middleButtonPushed();
            }
        }
        else
        {
            if (t2 == true)
            {
                t2 = false;
            }
        }

        if (GUI__isInArea(pos.x, pos.y, T3_X, T3_Y, SIZE_X, SIZE_Y))
        {
            if (t3 == false)
            {
                t3 = true;

                pages[currentPage]->deActivate();
                currentPage++;
                if (currentPage >= pageCount)
                {
                    currentPage = 0;
                }
                Serial.printf("GUI INF T3 Pos x %d, Pos y %d , Page: %d\r\n", pos.x, pos.y, currentPage);
                pages[currentPage]->activate();
                //GUI__header(pages[currentPage]->getHeader().c_str());
            }
        }
        else
        {
            if (t3 == true)
            {
                t3 = false;
            }
        }
    }
    else
    {
        t1 = false;
        t2 = false;
        t3 = false;
    }

    return (touchActive);
}

/**
 * Function checks if image is available and updtodate
 * if not it will download the image file from imgServer
 ***/
bool GUI_CheckImage(String path)
{
    bool retVal = true;
    if ((!SD.exists(path)) || (forceDownload))
    {
        if (SD.exists(path))
        {
            SD.remove(path);
        }
        //M5.Lcd.print("Downloading Image\r\n");
        Serial.printf("GUI INF File: %s not found, downloading from %s", path.c_str(), imgServer.c_str());
        File file = SD.open(path, "a");
        HTTPClient http;

        http.begin(imgServer + path); //Specify the URL and certificate
        int httpCode = http.GET();    //Make the request

        if (httpCode == HTTP_CODE_OK)
        { //Check for the returning code
            http.writeToStream(&file);
        }
        else
        {
            retVal = false;
            Serial.printf("GUI ERR File: %s failed to download", path.c_str());
            //M5.Lcd.print("Downloading Image failed\r\n");
        }

        file.close();
        Serial.printf("GUI INF File: %s downloaded and stored", path.c_str());
        http.end(); //Free the resources
    }
}