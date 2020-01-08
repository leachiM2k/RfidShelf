#ifndef ShelfButtons_h
#define ShelfButtons_h

#include "EasyButtonMCP.h"
#include "ShelfConfig.h"
#include "ShelfPins.h"
#include "ShelfPlayback.h"

#include <Adafruit_MCP23017.h>

class ShelfButtons {
  public:
    ShelfButtons(ShelfPlayback &playback);
    void begin();
    void work();
    static void handleVolumeDown();
    static void handleVolumeUp();
    static void handlePause();
    static void handlePrev();
    static void handleSkip();
  private:
    static ShelfButtons *_instance;
    ShelfPlayback &_playback;
    unsigned long _lastAnalogCheck = 0L;
    Adafruit_MCP23017 _mcp;
    EasyButtonMCP _volumeUp;
    EasyButtonMCP _volumeDown;
    EasyButtonMCP _pauseButton;
    EasyButtonMCP _prevButton;
    EasyButtonMCP _skipButton;
    uint8_t _lastAnalogVolume = DEFAULT_VOLUME;
    void _handleVolume();
};

#endif // ShelfButtons_h