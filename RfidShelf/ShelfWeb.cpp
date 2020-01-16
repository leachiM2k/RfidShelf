#include "ShelfWeb.h"

// This sucks - Maybe refactor ShelfWeb to singleton
ShelfWeb *ShelfWeb::_instance;

ShelfWeb::ShelfWeb(ShelfPlayback &playback, ShelfRfid &rfid, SdFat &sd, NTPClient &timeClient) : _playback(playback), _rfid(rfid), _SD(sd), _timeClient(timeClient), _server(80) {
  _instance = this;
}

void ShelfWeb::defaultCallback(AsyncWebServerRequest *request) {
  _instance->_handleDefault(request);
}

void ShelfWeb::fileUploadCallback(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  _instance->_handleFileUpload(request, filename, index, data, len, final);
}

void ShelfWeb::begin() {
  _server.on("/", HTTP_POST, defaultCallback, fileUploadCallback);
  _server.onNotFound(defaultCallback);

  _server.begin();

  MDNS.addService("http", "tcp", 80);
}

void ShelfWeb::_returnOK(AsyncWebServerRequest *request) {
  request->send_P(200, "text/plain", NULL);
}

void ShelfWeb::_returnHttpStatus(AsyncWebServerRequest *request, uint16_t statusCode, const char *msg) {
  request->send_P(statusCode, "text/plain", msg);
}

void ShelfWeb::_sendHTML(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", ShelfHtml::INDEX, ShelfHtml::INDEX_SIZE);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}

void ShelfWeb::_sendJsonStatus(AsyncWebServerRequest *request) {
  char output[512] = "{\"playback\":\"";
  char buffer[101];

  if(_playback.playbackState() == PLAYBACK_FILE) {
    strcat(output, "FILE\"");
  } else if(_playback.playbackState() == PLAYBACK_PAUSED) {
    strcat(output, "PAUSED\"");
  } else {
    strcat(output, "NO\"");
  }

  if(_rfid.hasActivePairing) {
    nfcTagObject pairingConfig = _rfid.getPairingConfig();
    strcat(output, ",\"pairing\":\"");
    strcat(output, pairingConfig.folder);
    strcat(output, "\"");
  }

  if(_playback.playbackState() != PLAYBACK_NO) {
    _playback.currentFile(buffer, sizeof(buffer));
    strcat(output, ",\"currentFile\":\"");
    strcat(output, buffer);
    strcat(output, "\"");
  }

  strcat(output, ",\"volume\":");
  snprintf(buffer, sizeof(buffer), "%d", 50 - _playback.volume());
  strcat(output, buffer);

  if (_SD.exists("/patches.053")) {
    strcat(output, ",\"patch\":true");
  } else {
    strcat(output, ",\"patch\":false");
  }

  if(_playback.isNight()) {
    strcat(output, ",\"night\":true");
  } else {
    strcat(output, ",\"night\":false");
  }

  if(_playback.defaultShuffleMode) {
    strcat(output, ",\"shuffle\":true");
  } else {
    strcat(output, ",\"shuffle\":false");
  }

  if(_playback.isShuffle()) {
    strcat(output, ",\"currentShuffle\":true");
  } else {
    strcat(output, ",\"currentShuffle\":false");
  }

  strcat(output, ",\"time\":");
  snprintf(buffer, sizeof(buffer), "%lu", _timeClient.getEpochTime());
  strcat(output, buffer);

  // This is too slow on bigger cards, so it needs to be moved somewhere else
  /*strcat(output, ",\"sdfree\":");
  snprintf(buffer, sizeof(buffer), "%u", (uint32_t)(0.000512*_SD.vol()->freeClusterCount()*_SD.vol()->blocksPerCluster()));
  strcat(output, buffer);

  strcat(output, ",\"sdsize\":");
  snprintf(buffer, sizeof(buffer), "%u", (uint32_t)(0.000512*_SD.card()->cardCapacity()));
  strcat(output, buffer);*/


  strcat(output, ",\"version\":");
  snprintf(buffer, sizeof(buffer), "\"%d.%d\"", MAJOR_VERSION, MINOR_VERSION);
  strcat(output, buffer);
  strcat(output, "}");

  request->send_P(200, "application/json", output);
}

void ShelfWeb::_sendJsonFS(AsyncWebServerRequest *request, const char *path) {
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->print(F("{\"fs\":["));
  request->send_P(200, "application/json", "{\"fs\":[");

  SdFile dir;
  dir.open(path, O_READ);
  dir.rewind();

  SdFile entry;

  char buffer[101];

  bool first = true;

  while (entry.openNext(&dir, O_READ)) {
    if(first) {
      first = false;
      response->print(F("{\"name\":\""));
    } else {
      response->print(F(", {\"name\":\""));
    }
    entry.getName(buffer, sizeof(buffer));
    // TODO encode special characters
    response->print(buffer);
    if (entry.isDir()) {
      response->print(F("\""));
    } else {
      response->print(F("\",\"size\":"));
      snprintf(buffer, sizeof(buffer), "%lu", (unsigned long) entry.fileSize());
      response->print(buffer);
    }
    entry.close();
    response->print(F("}"));
  }
  response->print(F("],"));
  response->printf("\"path\":\"%s\"", path);
  response->print(F("}"));
  response->print(F(""));
  request->send(response);

  dir.close();
}

bool ShelfWeb::_loadFromSdCard(AsyncWebServerRequest *request, const char *path) {
  File dataFile = _SD.open(path);

  if (!dataFile) {
    Sprintln(F("File not open"));
    _returnHttpStatus(request, 404, "Not found");
    return false;
  }

  if (dataFile.isDir()) {
    _sendJsonFS(request, path);
  } else {
    request->send(dataFile, "application/octet-stream");
    /*
    if (request->send(dataFile, "application/octet-stream") != dataFile.size()) {
      Sprintln(F("Sent less data than expected!"));
    }
    */
  }
  dataFile.close();
  return true;
}

void ShelfWeb::_downloadPatch(AsyncWebServerRequest *request) {
  Sprintln(F("Starting patch download"));
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  // do not validate certificate
  client->setInsecure();
  HTTPClient httpClient;
  httpClient.begin(*client, VS1053_PATCH_URL);
  int httpCode = httpClient.GET();
  if (httpCode < 0) {
    _returnHttpStatus(request, 500, httpClient.errorToString(httpCode).c_str());
    return;
  }
  if (httpCode != HTTP_CODE_OK) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "invalid response code: %d", httpCode);
    _returnHttpStatus(request, 500, buffer);
    return;
  }
  int len = httpClient.getSize();
  WiFiClient* stream = httpClient.getStreamPtr();
  uint8_t buffer[128] = { 0 };
  SdFile patchFile;
  patchFile.open("/patches.053", O_WRITE | O_CREAT);
  while (httpClient.connected() && (len > 0 || len == -1)) {
    // get available data size
    size_t size = stream->available();
    if (size) {
      // read til buffer is full
      int c = stream->readBytes(buffer, ((size > sizeof(buffer)) ? sizeof(buffer) : size));
      // write the buffer to our patch file
      if (patchFile.isOpen()) {
        patchFile.write(buffer, c);
      }
      // reduce len until we the end (= zero) if len not -1
      if (len > 0) {
        len -= c;
      }
    }
    delay(1);
  }
  if (patchFile.isOpen()) {
    patchFile.close();
  }
  _returnOK(request);
  ESP.restart();
}

void ShelfWeb::_handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  // Upload always happens on /
  if (request->url() != "/") {
    Sprintln(F("Invalid upload URI"));
    return;
  }

  if(!index){
    if (!Adafruit_VS1053_FilePlayer::isMP3File((char *)filename.c_str())) {
      Sprint(F("Not a MP3: ")); Sprintln(filename);
      return;
    }

    if (!filename.startsWith("/")) {
      Sprintln(F("Invalid upload target"));
      return;
    }

    if (_SD.exists(filename.c_str())) {
      Sprintln("File " + filename + " already exists. Skipping");
      return;
    }

    _uploadFile.open(filename.c_str(), O_WRITE | O_CREAT);
    _uploadStart = millis();
    Sprint(F("Upload start: "));
    Sprintln(filename);
  } else if (final) {
    if (_uploadFile.isOpen()) {
      _uploadFile.close();
      Sprint(F("Upload end: ")); Sprintln(index + len);
      Sprint(F("Took: ")); Sprintln(((millis()-_uploadStart)/1000));
    }
  } else {
    if (_uploadFile.isOpen()) {
      _uploadFile.write(data, len);
      //Sprint(F("Upload write: "));
      //Sprintln(upload.currentSize);
    }
  }
}

void ShelfWeb::_handleDefault(AsyncWebServerRequest *request) {
  String path = request->urlDecode(request->url());
  Sprintf(F("Request to: %s\n"), path.c_str());
  if (request->method() == HTTP_GET) {
    if (request->hasArg("status")) {
      _sendJsonStatus(request);
      return;
    } else if(path == "/" && !request->hasArg("fs")) {
      _sendHTML(request);
      return;
    } else {
      _loadFromSdCard(request, path.c_str());
      return;
    }
  } else if (request->method() == HTTP_DELETE) {
    if (path == "/" || !_SD.exists(path.c_str())) {
      _returnHttpStatus(request, 400, "Bad path");
      return;
    }

    SdFile file;
    file.open(path.c_str());
    if (file.isDir()) {
      if(!file.rmRfStar()) {
        Sprintln(F("Could not delete folder"));
      }
    } else {
      if(!_SD.remove(path.c_str())) {
        Sprintln(F("Could not delete file"));
      }
    }
    file.close();
    _returnOK(request);
    return;
  } else if (request->method() == HTTP_POST) {
    if (request->hasArg("newFolder")) {
      Sprint(F("Creating folder ")); Sprintln(request->arg("newFolder"));
      _SD.mkdir(request->arg("newFolder").c_str());
      _returnOK(request);
      return;
    } else if (request->hasArg("ota")) {
      Sprint(F("Starting OTA from ")); Sprintln(request->arg("ota"));
      std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
      // do not validate certificate
      client->setInsecure();
      t_httpUpdate_return ret = ESPhttpUpdate.update(*client, request->arg("ota"));
      switch (ret) {
        case HTTP_UPDATE_FAILED:
          Sprintf("HTTP_UPDATE_FAILD Error (%d): ", ESPhttpUpdate.getLastError());
          Sprintln(ESPhttpUpdate.getLastErrorString().c_str());
          _returnHttpStatus(request, 500, "Update failed, please try again");
          return;
        case HTTP_UPDATE_NO_UPDATES:
          Sprintln(F("HTTP_UPDATE_NO_UPDATES"));
          break;
        case HTTP_UPDATE_OK:
          Sprintln(F("HTTP_UPDATE_OK"));
          break;
      }
      _returnOK(request);
      return;
    } else if (request->hasArg("downloadpatch")) {
      _downloadPatch(request);
      return;
    } else if (request->hasArg("stop")) {
      _playback.stopPlayback();
      _sendJsonStatus(request);
      return;
    } else if (request->hasArg("pause")) {
      _playback.pausePlayback();
      _sendJsonStatus(request);
      return;
    } else if (request->hasArg("resume")) {
      _playback.playingByCard = false;
      _playback.resumePlayback();
      _sendJsonStatus(request);
      return;
    } else if (request->hasArg("skip")) {
      _playback.playingByCard = false;
      _playback.skipFile();
      _sendJsonStatus(request);
      return;
    } else if (request->hasArg("volumeUp")) {
      _playback.volumeUp();
      _sendJsonStatus(request);
      return;
    } else if (request->hasArg("volumeDown")) {
      _playback.volumeDown();
      _sendJsonStatus(request);
      return;
    } else if (request->hasArg("toggleNight")) {
      if(_playback.isNight()) {
        _playback.stopNight();
      } else {
        _playback.startNight();
      }
      _sendJsonStatus(request);
      return;
    } else if (request->hasArg("toggleShuffle")) {
      if(_playback.defaultShuffleMode) {
        _playback.defaultShuffleMode = false;
        _playback.stopShuffle();
      } else {
        _playback.defaultShuffleMode = true;
        _playback.startShuffle();
      }
      _sendJsonStatus(request);
      return;
    } else if (request->url() == "/") {
      Sprintln(F("Probably got an upload request"));
      _returnOK(request);
      return;
    } else if (_SD.exists(path.c_str())) {
      // <= 17 here because leading "/"" is included
      if (request->hasArg("write") && path.length() <= 17) {
        const char *target = path.c_str();
        const uint8_t volume = (uint8_t)request->arg("volume").toInt();
        uint8_t repeat = 0;       // keep configured setting
        uint8_t shuffle = 0;      // keep configured setting
        uint8_t stopOnRemove = 0; // keep configured setting
        if(request->arg("repeat").equals("1") || request->arg("repeat").equals("0")) {
          repeat = 2 + request->arg("repeat").toInt();
        }
        if(request->arg("shuffle").equals("1") || request->arg("shuffle").equals("0")) {
          repeat = 2 + request->arg("shuffle").toInt();
        }
        if(request->arg("stopOnRemove").equals("1") || request->arg("stopOnRemove").equals("0")) {
          repeat = 2 + request->arg("stopOnRemove").toInt();
        }
        // Remove leading "/""
        target++;
        if (_rfid.startPairing(target, volume, repeat, shuffle, stopOnRemove)) {
          _sendJsonStatus(request);
          return;
        }
      } else if (request->hasArg("play") && _playback.switchFolder(path.c_str())) {
        _playback.startPlayback();
        _playback.playingByCard = false;
        _sendJsonStatus(request);
        return;
      } else if (request->hasArg("playfile")) {
        char* pathCStr = (char *)path.c_str();
        char* folderRaw = strtok(pathCStr, "/");
        char* file = strtok(NULL, "/");

        if((folderRaw != NULL) && (file != NULL) && strlen(folderRaw) < 90) {

          char folder[100] = "/";
          strcat(folder, folderRaw);
          strcat(folder, "/");

          if(_playback.switchFolder(folder)) {
            _playback.startFilePlayback(folderRaw, file);
            _playback.playingByCard = false;
            _sendJsonStatus(request);
            return;
          }
        }
      }
    }
  }

  // 404 otherwise
  _returnHttpStatus(request, 404, F("Not found"));
  Sprintln("404: " + path);
}

void ShelfWeb::work() {
}
