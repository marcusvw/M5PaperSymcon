#include <M5EPD.h>
#include "WIF.h"
#include <Preferences.h>
#include "SLI.h"
#include <ArduinoJson.h>
#include "RPC.h"
#include "GUI.h"

M5EPD_Canvas canvas(&M5.EPD);
char temStr[10];
char humStr[10];

float tem;
float hum;

void setup()
{

  /**Initialize M5Stack stuff*/
  M5.begin();
  M5.SHT30.Begin();
  M5.EPD.SetRotation(0);
  M5.EPD.Clear(true);
  canvas.createCanvas(400, 300);
  canvas.setTextSize(2);
  canvas.drawString("Booting System ... V0.0.1", 0, 100);
  canvas.pushCanvas(0, 100, UPDATE_MODE_A2);
  delay(2000);
  Serial.setTimeout(2000);
  Serial.flush();

  /** Reset on request the config of the modules below**/
  Serial.println("Reset?(y)");
  String res = Serial.readString();
  res.trim();
  if (res == "y")
  {
    Serial.println("Reset Config");
    WIF_resetConfig();
    JsonRPC::resetConfig();
  }

  /**Init WIFI**/
  WIF_init();
  /**Init GUI**/
  GUI_Init();
  delay(100);
}

void loop()
{
  /** GUI Loop does all the job in regards to Pages/Touch etc.**/
  GUI_Loop();
}
