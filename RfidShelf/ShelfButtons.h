#ifndef ShelfButtons_h
#define ShelfButtons_h

#include "ShelfConfig.h"
#include "ShelfPins.h"
#include "ShelfPlayback.h"

#include "EasyButton.h"

class ShelfButtons {
  public:
    ShelfButtons(ShelfPlayback &playback);
    void begin();
    void work();
    static void handlePause();
    static void handleSkip();
  private:
    static ShelfButtons *_instance;
    ShelfPlayback &_playback;
    EasyButton _pauseButton;
    EasyButton _skipButton;
    unsigned long _lastAnalogCheck = 0L;
    uint8_t _lastAnalogVolume = DEFAULT_VOLUME;
    void _handleVolume();
};

#endif // ShelfButtons_h