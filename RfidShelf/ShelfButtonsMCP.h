#ifndef ShelfButtonsMCP_h
#define ShelfButtonsMCP_h

#include "ShelfConfig.h"
#include "ShelfPins.h"
#include "ShelfPlayback.h"

#include "EasyButtonMCP.h"
#include <Adafruit_MCP23017.h>

class ShelfButtonsMCP {
  public:
    ShelfButtonsMCP(ShelfPlayback &playback);
    void begin();
    void work();
    static void handleVolumeDown();
    static void handleVolumeUp();
    static void handlePause();
    static void handlePrev();
    static void handleSkip();
  private:
    static ShelfButtonsMCP *_instance;
    ShelfPlayback &_playback;
    Adafruit_MCP23017 _mcp;
    EasyButtonMCP _skipButton;
    EasyButtonMCP _prevButton;
    EasyButtonMCP _pauseButton;
    EasyButtonMCP _volumeUp;
    EasyButtonMCP _volumeDown;
    unsigned long _lastAnalogCheck = 0L;
    uint8_t _lastAnalogVolume = DEFAULT_VOLUME;
    void _handleVolume();
};

#endif // ShelfButtons_h