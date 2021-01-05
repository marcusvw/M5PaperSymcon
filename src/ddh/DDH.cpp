
#include <FS.h>
#include <SD.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include <ArduinoOTA.h>

bool DDH__UseSDCard = true;
bool DDH__forceDownload = false;
String DDH__ImageServer("");
String DDH__OTAHostName("");

void DDH_Init(String paramImgServer, bool paramForceDownload, bool paramUseSD, uint32_t checkSWVersion, String paramOTAHostName)
{
    DDH__UseSDCard = paramUseSD;
    DDH__forceDownload = paramForceDownload;
    DDH__ImageServer = paramImgServer;
    DDH__OTAHostName = paramOTAHostName;
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";
        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("DDH INF Start updating " + type);
    });
    ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("DDH INF Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("DD ERR Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
            Serial.println("End Failed");
    });
    ArduinoOTA.setHostname(DDH__OTAHostName.c_str());
    ArduinoOTA.begin();
}
/**
 * Function checks if image is available and updtodate
 * if not it will download the image file from imgServer
 ***/
bool DDH_CheckImage(String path)
{
    bool retVal = true;
    if ((!SD.exists(path)) || (DDH__forceDownload))
    {
        if (SD.exists(path))
        {
            SD.remove(path);
        }
        //M5.Lcd.print("Downloading Image\r\n");
        Serial.printf("GUI INF File: %s not found, downloading from %s", path.c_str(), DDH__ImageServer.c_str());
        File file = SD.open(path, "a");
        HTTPClient http;

        http.begin(DDH__ImageServer + path); //Specify the URL and certificate
        int httpCode = http.GET();           //Make the request

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
void DDH__Loop()
{
    ArduinoOTA.handle();
    yield();
}
