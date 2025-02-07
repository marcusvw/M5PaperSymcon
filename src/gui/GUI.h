#pragma once
#include <Arduino.h>
#define OFFSET 5
#define OFFSET_Y 40
#define PAGE_PADDING_Y 20
#define SIZE_X 100
#define SIZE_Y 50 
#define T1_X (OFFSET)
#define T1_Y (240)
#define T2_X (T1_X+OFFSET+SIZE_X)
#define T2_Y (240)
#define T3_X (T2_X+OFFSET+SIZE_X)
#define T3_Y (240)
#define CHAPTER_UP true
#define CHAPTER_DOWN false
#define SLEEP_TIMEOUT_LONG 10000 /*ms*/
#define I2C_ADDR_AXP 0x34
#define I2C_ADDR_TOUCH 0x38
#define CST_POWER_MODE_ACTIVE (0x00)
#define CST_POWER_MODE_MONITOR (0x01)
#define CST_POWER_MODE_HIBERNATE (0x03) // deep sleep
#define NUM_PAGES 6
#define PAGE_WIDTH 320
#define PAGE_HEIGHT 240
#define NUM_CHAPTERS_MAX 10
#define NUM_PAGES_MAX (NUM_CHAPTERS_MAX*NUM_PAGES)

bool GUI__isInArea(int xT, int yT,int x,int y, int sizeX, int sizeY);
void GUI_Init();
void GUI_Loop();
bool GUI__checkButtons();
void GUI__header(const char *string);
bool GUI_CheckImage(String path);
void GUI_switchChapter(bool dir);
bool GUI_cachedUpdate();

