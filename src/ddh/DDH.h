#pragma once
#include <Arduino.h>
void DDH__Loop();
void DDH_Init(String paramImgServer, bool paramForceDownload, bool paramUseSD, uint32_t checkSWVersion, String paramOTAHostName);
bool DDH_CheckImage(String path);

