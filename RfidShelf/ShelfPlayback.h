#ifndef ShelfPlayback_h
#define ShelfPlayback_h

#include "ShelfPins.h"
#include "ShelfConfig.h"
#include <Adafruit_VS1053.h>
#include <ESP8266HTTPClient.h>
#include <SdFat.h>
#include <BoolArray.h>

enum PlaybackState {PLAYBACK_NO, PLAYBACK_FILE, PLAYBACK_PAUSED};

class ShelfPlayback {
  public:
    ShelfPlayback(SdFat &sd) :
      _musicPlayer(Adafruit_VS1053_FilePlayer(BREAKOUT_RESET, BREAKOUT_CS, BREAKOUT_DCS, DREQ, SD_CS)),
      _SD(sd)
      {};
    void begin();
    const bool switchFolder(const char *folder);
    void startPlayback();
    void startFilePlayback(const char* folder, const char* file);
    void skipFile();
    void pausePlayback();
    void resumePlayback();
    void togglePause();
    void stopPlayback();
    const uint8_t volume() {return _volume;};
    void volume(uint8_t volume);
    void volumeDown();
    void volumeUp();
    void startNight();
    const bool isNight();
    void stopNight();
    void startRandom();
    const bool isRandom();
    void stopRandom();
    void setBassAndTreble(uint8_t trebleAmplitude, uint8_t trebleFreqLimit, uint8_t bassAmplitude, uint8_t bassFreqLimit);
    const PlaybackState playbackState() {return _playing;};
    void currentFile(char *name, size_t size);
    void currentFolder(char *name, size_t size);
    void work();
    bool playingByCard = true;
  private:
    uint8_t _volume = DEFAULT_VOLUME;
    PlaybackState _playing = PLAYBACK_NO;
    Adafruit_VS1053_FilePlayer _musicPlayer;
    SdFat &_SD;
    SdFile _currentFolder;
    uint16_t _currentFolderFileCount;
    char _currentFile[100];
    const bool patchVS1053();
    bool _nightMode = false;
    // Night mode timeouts NIGHT_TIMEOUT minutes after playback ends, so we need to keep count
    unsigned long _lastNightActivity;
    bool _randomMode = false;
    uint16_t _randomPlaybackCount;
    // Store playback history for random playback to prevent repititions
    BoolArray _randomHistory;
};

#endif // ShelfPlayback_h
