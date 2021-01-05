#include "PAG/PAG.h"
#include "../rpc/RPC.h"
#include "../ddh/DDH.h"
#include "SLI/SLI.h"
#include <ArduinoJson.h>
#include "GUI.h"
#include <M5EPD.h>
#include <Preferences.h>
#include "../wif/WIF.h"
#include "BT4/BT4.h"
static Page *pages[NUM_PAGES_MAX];
static uint8_t pageCount = 0;
static uint8_t currentChapter = 0;
static uint32_t lastActive = 0;
static bool forceDownload = false;
static uint32_t sleepTimeout = SLEEP_TIMEOUT_LONG;
static String imgServer = "";
static uint32_t confVers = 0;
static uint32_t touchReleaseCounter = 0;
static uint32_t notAvailableCounter = 0;
PAG_pos_t dummyPos = {-1, -1};
PAG_pos_t positionTemplate[NUM_PAGES] = {{PAGE_WIDTH * 0, (PAGE_HEIGHT * 0) + OFFSET_Y}, {PAGE_WIDTH * 1, (PAGE_HEIGHT * 0) + OFFSET_Y}, {PAGE_WIDTH * 2, (PAGE_HEIGHT * 0) + OFFSET_Y}, {PAGE_WIDTH * 0, (PAGE_HEIGHT * 1) + OFFSET_Y + PAGE_PADDING_Y}, {PAGE_WIDTH * 1, (PAGE_HEIGHT * 1) + OFFSET_Y + PAGE_PADDING_Y}, {PAGE_WIDTH * 2, (PAGE_HEIGHT * 1) + OFFSET_Y + PAGE_PADDING_Y}};
Preferences GUI__preferences;
PAG_pos_t pos;
void GUI_Init()
{
    String hostname;
    bool useSDCard=false;
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
    hostname = doc["hostname"].as<String>();
    sleepTimeout = doc["sleepTimeout"].as<uint32_t>();
    useSDCard =doc["useSDCard"].as<boolean>();
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
    DDH_Init(imgServer,forceDownload, useSDCard, 0, hostname);
    WiFi.setHostname(hostname.c_str());
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
            pages[pageCount] = new SliderPage((JsonObject)(doc["elements"][x]), positionTemplate[pageIndex],useSDCard);
            pageCount++;
        }
        // Type is 4 Button Page
        else if (type == "BT4")
        {
            Serial.printf("GUI INF B4T Config: %s \r\n", doc["elements"][x]["head"].as<String>().c_str());
            pages[pageCount] = new Button4Page((JsonObject)(doc["elements"][x]), positionTemplate[pageIndex],useSDCard);
            pageCount++;
        }
        pageIndex++;
        if (pageIndex > NUM_PAGES)
        {
            pageIndex = 0;
        }
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
                lastActive = millis();
            }
            Serial.printf("GUI INF Touch: %d, %d\r\n", pos.x, pos.y);
        }
        else
        {
            pos.x = -1;
            pos.y = -1;
            Serial.printf("GUI INF Touch released\r\n");
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
        }
    }
    for (int x = 0; x < NUM_PAGES; x++)
    {
        if (pages[x] != NULL)
        {

            if (pos.x == -1)
            {

                pages[x]->handleInput(dummyPos);
            }
            else
            {
                if ((pos.x >= positionTemplate[x].x) && (pos.x < (positionTemplate[x].x + PAGE_WIDTH)) &&
                    (pos.y >= positionTemplate[x].y) && (pos.y < (positionTemplate[x].y + PAGE_HEIGHT)))
                {
                    Serial.printf("GUI INF Selected Page %d in Chapter %d", x, currentChapter);
                    PAG_pos_t locPos = pos;
                    locPos.x -= positionTemplate[x].x;
                    locPos.y -= positionTemplate[x].y;
                    pages[x]->handleInput(locPos);
                }
                else
                {
                    pages[x]->handleInput(dummyPos);
                }
            }
        }
    }
    GUI__checkButtons();
    DDH__Loop();
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
bool GUI__checkButtons()
{
    if (M5.BtnL.wasPressed())
    {
        Serial.printf("BtnL wasPressed!");
    }
    if (M5.BtnR.wasPressed())
    {
        Serial.printf("BtnR wasPressed!");
    }
    M5.update();
}


bool GUI_CheckImage(String path)
{
   DDH_CheckImage(path);
}