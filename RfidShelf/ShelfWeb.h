#ifndef ShelfWeb_h
#define ShelfWeb_h

#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266httpUpdate.h>
#include <Adafruit_VS1053.h>
#include <SD.h>
#include <NTPClient.h>
#include "ShelfConfig.h"
#include "ShelfPlayback.h"
#include "ShelfRfid.h"
#include "ShelfVersion.h"
#include "ShelfHtml.h"

class ShelfWeb {
  public:
    ShelfWeb(ShelfPlayback &playback, ShelfRfid &rfid, SDClass &sd, NTPClient &timeClient);
    void begin();
    void work();
    static void defaultCallback(AsyncWebServerRequest *request);
    static void fileUploadCallback(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
  private:
    static ShelfWeb *_instance;
    ShelfPlayback &_playback;
    ShelfRfid &_rfid;
    SDClass _SD;
    NTPClient &_timeClient;
    AsyncWebServer _server;
    File _uploadFile;
    uint32_t _uploadStart;
    void _returnOK(AsyncWebServerRequest *request);
    void _returnHttpStatus(AsyncWebServerRequest *request, uint16_t statusCode, const char *msg);
    void _sendHTML(AsyncWebServerRequest *request);
    void _sendJsonStatus(AsyncWebServerRequest *request);
    void _sendJsonFS(AsyncWebServerRequest *request, const char *path);
    bool _loadFromSdCard(AsyncWebServerRequest *request, const char *path);
    void _handleWriteRfid(const char *folder);
    void _handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    void _handleDefault(AsyncWebServerRequest *request);
    void _downloadPatch(AsyncWebServerRequest *request);
    void _updateOTA();
};

#endif // ShelfWeb_h
