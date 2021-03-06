#include <Arduino.h>
#include "ShelfPins.h"
#include "ShelfConfig.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <SdFat.h>
#include <SD.h>
#include <ESP8266mDNS.h>
#include <time.h>
#include <coredecls.h>        // settimeofday_cb()
#include "ShelfPlayback.h"
#include "ShelfRfid.h"
#include "ShelfWeb.h"

#ifdef ARDUINO_OTA_ENABLE
#include <ArduinoOTA.h>
#endif

#ifdef BUTTONS_ENABLE
#include "ShelfButtons.h"
#endif

#ifdef PUSHOVER_ENABLE
#include "ShelfPushover.h"
#endif


WiFiManager wifiManager;

sdfat::SdFat sdCard;

ShelfPlayback playback(sdCard);
ShelfRfid rfid(playback);
ShelfWeb webInterface(playback, rfid, sdCard);
#ifdef BUTTONS_ENABLE
ShelfButtons buttons(playback);
#endif
#ifdef PUSHOVER_ENABLE
ShelfPushover pushover;
#endif

void timeCallback() {
      Sprint("Time updated: ");
      time_t now = time(nullptr);
      Sprintln(ctime(&now));
    }

void setup() {
#ifdef DEBUG_ENABLE
  Serial.begin(115200);
#endif
  Sprintln();
  Sprintln(F("Starting ..."));

  // Init SPI SS pins
  pinMode(RC522_CS, OUTPUT);
  digitalWrite(RC522_CS, HIGH);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  pinMode(BREAKOUT_CS, OUTPUT);
  digitalWrite(BREAKOUT_CS, HIGH);
  pinMode(BREAKOUT_DCS, OUTPUT);
  digitalWrite(BREAKOUT_DCS, HIGH);

  randomSeed(analogRead(A0));

  // Load config
  ShelfConfig::init();

  rfid.begin();
    
  //Initialize the SdCard.
  if (!sdCard.begin(SD_CS) || !sdCard.chdir("/")) {
    Sprintln(F("Could not initialize SD card width SdFat"));
    sdCard.initErrorHalt();
  }
  Sprintln(F("SDFat ready"));
  if(!SD.begin(SD_CS)){
    Sprintln(F("Could not initialize SD card with SD"));
  }
  Sprintln(F("SD ready"));


  playback.begin();

  if(strcmp(ShelfConfig::config.ntpServer, "") != 0) {
    settimeofday_cb(timeCallback);
    configTime(ShelfConfig::config.timezone, ShelfConfig::config.ntpServer);
  }


  Sprint("Hostname: "); Sprintln(ShelfConfig::config.hostname);
  WiFi.hostname(ShelfConfig::config.hostname);

  wifiManager.setConfigPortalTimeout(3 * 60);
  if (!wifiManager.autoConnect("MP3-SHELF-SETUP", "lacklack")) {
    Sprintln(F("Setup timed out, starting AP"));
    WiFi.mode(WIFI_AP);
    if (WiFi.softAP("MP3-SHELF", "lacklack")) {
      Sprintln(F("Soft-AP is set up"));
    } else {
      Sprintln(F("Soft-AP setup failed"));
    }
  }

  if(WiFi.isConnected()) {
    Sprint(F("Connected! IP address: ")); Sprintln(WiFi.localIP());
  }

  if (!MDNS.begin(ShelfConfig::config.hostname)) {
    Sprintln("Error setting up MDNS responder!");
  }

#ifdef ARDUINO_OTA_ENABLE
  // TODO move into EEPROM config
  ArduinoOTA.setPassword((const char *)"shelfshelf");
  ArduinoOTA.begin();
#endif

  webInterface.begin();

#ifdef BUTTONS_ENABLE
  buttons.begin();
#endif

  playback.startFilePlayback("", "ready.mp3");

  Sprintln(F("Init done"));
}

void loop() {
#ifdef BUTTONS_ENABLE
  buttons.work();
#endif

#ifdef ARDUINO_OTA_ENABLE
  ArduinoOTA.handle();
#endif

  MDNS.update();

  playback.work();

  rfid.handleRfid();

#ifdef PUSHOVER_ENABLE
  pushover.sendPoweredNotification();
#endif

  webInterface.work();
}