// ######################################################################################################
// #################################### Plugin 069: TFT ST7735 status display ###########################
//
// originally based on plugin 36
// the goal is to offer a status display for values
// like room temperature, window/door sensors etc. from mqtt
//

#define PLUGIN_069
#define PLUGIN_ID_069         69
#define PLUGIN_NAME_069       "Display - ST7735"
#define PLUGIN_VALUENAME1_069 "TFT"

// while these offer well-done abstractions they horribly inefficient and don't offer double buffering
// nevertheless, thanks Adafruit
#include "Adafruit_GFX.h"    // Core graphics library
#include "Adafruit_ST7735.h" // Hardware-specific library
//#include <SPI.h>

#define TFT_CS     D0
#define TFT_RST    D1
#define TFT_DC     D2

// use default ESP8266 SPI pins D5 and D7 (hopefully this does hw SPI)
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

// max. number of bar graph values
#define MAX_BARS 6

boolean Plugin_069(byte function, struct EventStruct *event, String& string) {
  boolean success = false;

  static byte displayTimer = 0;
  static byte frameCounter = 0;				// need to keep track of framecounter from call to call
  static boolean firstcall = true;			// This is used to clear the init graphic on the first call to read

  switch (function) {
    case PLUGIN_DEVICE_ADD: {
        Device[++deviceCount].Number = PLUGIN_ID_069;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE; // NONE - output only
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 0;
        Device[deviceCount].SendDataOption = false;
        Device[deviceCount].TimerOption = false;
        break;
      }

    case PLUGIN_GET_DEVICENAME: {
        string = F(PLUGIN_NAME_069);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES: {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_069));  // OnOff
        break;
      }

    case PLUGIN_WEBFORM_LOAD: {
      byte choice1 = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
      String options1[2];
      options1[0] = F("Normal");
      options1[1] = F("Rotated");
      int optionValues1[2] = { 1, 2 };
      addFormSelector(string, F("Rotation"), F("plugin_069_rotate"), 2, options1, optionValues1, choice1);

      byte choice2 = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
      String options2[MAX_BARS];
      options2[0] = F("1");
      options2[1] = F("2");
      options2[2] = F("3");
      options2[3] = F("4");
      options2[4] = F("5");
      options2[5] = F("6"); // TODO fixme loop
      int optionValues2[6] = { 1, 2, 3, 4, 5, 6};
      addFormSelector(string, F("Number of bars"), F("plugin_069_nbars"), MAX_BARS, options2, optionValues2, choice2);

      char deviceTemplate[MAX_BARS][32];
      LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));

      for (byte varNr = 0; varNr < MAX_BARS; varNr++)
      {
        addFormTextBox(string, String(F("Bar ")) + (varNr + 1), String(F("plugin_069_template")) + (varNr + 1), deviceTemplate[varNr], 32);
      }

      success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_069_rotate"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_069_nbars"));

        String argName;
        char deviceTemplate[MAX_BARS][32];

        for (byte varNr = 0; varNr < MAX_BARS; varNr++)
        {
          argName = F("plugin_069_template");
          argName += varNr + 1;
          strncpy(deviceTemplate[varNr], WebServer.arg(argName).c_str(), sizeof(deviceTemplate[varNr]));
        }

        SaveCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));

        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        char deviceTemplate[MAX_BARS][32];
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));

        tft.initR(INITR_BLACKTAB);   // initialize the ST7735S
        tft.setTextWrap(false); // Allow text to run off right edge
        tft.fillScreen(ST7735_BLACK);
        tft.setCursor(0, 30);
        tft.setTextColor(ST7735_RED);
        tft.setTextSize(1);

        P069_display_espname();
        P069_display_ip();
        P069_display_wifi_bars();

        frameCounter = 0;

        success = true;
        break;
      }

    case PLUGIN_TEN_PER_SECOND:
      {
        break;
      }

    case PLUGIN_ONCE_A_SECOND:
      {

        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        char deviceTemplate[MAX_BARS][32];
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));

        int nbars = Settings.TaskDevicePluginConfig[event->TaskIndex][1];

        tft.fillScreen(ST7735_BLACK);
        tft.setCursor(0, 30);

        P069_display_espname();
        P069_display_ip();
        P069_display_time();
        P069_display_wifi_bars();

        for (int bar = 0; bar < nbars; bar++){
          P069_display_bar(deviceTemplate[bar],0,40);
        }

        success = true;
        break;
      }

    case PLUGIN_WRITE:
      {

        break;
      }

  }
  return success;
}

// The screen is set up as 10 rows at the top for the header, 10 rows at the bottom for the footer and 44 rows in the middle for the scroll region

void P069_display_time() {
  String dtime = "%systime%";
  String newString = parseTemplate(dtime, 10);

  tft.setTextColor(ST7735_GREEN);
  tft.setTextSize(3);
  tft.println(newString.substring(0, 5));
}

void P069_display_espname() {

  String dtime = "%sysname%";
  String newString = parseTemplate(dtime, 10);
  newString.trim();

  tft.setTextColor(ST7735_GREEN);
  tft.setTextSize(2);
  tft.println(newString);
}
void P069_display_ip() {

  String dtime = "%ip%";
  String newString = parseTemplate(dtime, 10);
  newString.trim();

  tft.setTextColor(ST7735_GREEN);
  tft.setTextSize(1);
  tft.println(newString);
}

void P069_display_valuename(String valuename) {

  String newString = parseTemplate(valuename, 10);
  newString.trim();

  tft.setTextColor(ST7735_GREEN);
  tft.setTextSize(3);
  tft.println(newString);
}

void P069_display_wifi_bars() {

  int nbars_filled = (WiFi.RSSI() + 100) / 8;
  int x = 10;
  int y = 10;
  int size_x = 10;
  int size_y = 10;
  int nbars = 8;

    //	x,y are the x,y locations
    //	sizex,sizey are the sizes (should be a multiple of the number of bars)
    //	nbars is the number of bars and nbars_filled is the number of filled bars.

    //	We leave a 1 pixel gap between bars

    for (byte ibar = 1; ibar < nbars + 1; ibar++) {

      tft.writeFillRect(x + (ibar - 1)*size_x / nbars, y, size_x / nbars, size_y,0xffff);

      if (ibar <= nbars_filled) {
        tft.writeFillRect(x + (ibar - 1)*size_x / nbars, y + (nbars - ibar)*size_y / nbars, (size_x / nbars) - 1, size_y * ibar / nbars,0xffff);
      }
      else
      {
        tft.writeFillRect(x + (ibar - 1)*size_x / nbars, y + (nbars - ibar)*size_y / nbars, (size_x / nbars) - 1, size_y * ibar / nbars,0xffff);
      }
    }
}

void P069_display_bar(String valuename, int lowerbound, int upperbound){

}
