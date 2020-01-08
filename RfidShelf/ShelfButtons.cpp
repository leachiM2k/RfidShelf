#include "ShelfButtons.h"

// This sucks - Maybe refactor ShelfButtons to singleton
ShelfButtons *ShelfButtons::_instance;

ShelfButtons::ShelfButtons(ShelfPlayback &playback) :
  _playback(playback),
  _pauseButton(_mcp, PAUSE_BTN, 50, true, false),
  _skipButton(_mcp, SKIP_BTN, 50, true, false),
  _prevButton(_mcp, PREV_BTN, 50, true, false),
  _volumeDown(_mcp, VOLUME_DOWN_BTN, 50, true, false),
  _volumeUp(_mcp, VOLUME_UP_BTN, 50, true, false) {
    _instance = this;
};

void ShelfButtons::begin() {
  // Initialize the MCP23017
  _mcp.begin();
  _volumeDown.begin();
  _volumeDown.onPressed(handleVolumeDown);
  _volumeUp.begin();
  _volumeUp.onPressed(handleVolumeUp);
  _pauseButton.begin();
  _pauseButton.onPressed(handlePause);
  _prevButton.begin();
  _prevButton.onPressed(handlePrev);
  _skipButton.begin();
  _skipButton.onPressed(handleSkip);
}

void ShelfButtons::work() {
  _volumeDown.read();
  _volumeUp.read();
  _pauseButton.read();
  _prevButton.read();
  _skipButton.read();
  _handleVolume();
}

void ShelfButtons::handleVolumeDown() {
  _instance->_playback.volumeDown();
}

void ShelfButtons::handleVolumeUp() {
  _instance->_playback.volumeUp();
}

void ShelfButtons::handlePause() {
  _instance->_playback.togglePause();
}

void ShelfButtons::handlePrev() {
  // TODO: implement
}

void ShelfButtons::handleSkip() {
  _instance->_playback.skipFile();
}

void ShelfButtons::_handleVolume() {
  // Note: Analog volume always wins! Volume from cards/web will be ignored/overwritten

  if (millis() - _lastAnalogCheck < 500) {
    return;
  }
  _lastAnalogCheck = millis();

  uint8_t volume_new = analogRead(VOLUME) / 20; // Map 1024 roughly into our 0-50 volume
  if(volume_new != _lastAnalogVolume  || _lastAnalogVolume != _playback.volume()) {
    _playback.volume(volume_new);
    _lastAnalogVolume = _playback.volume();
  }
}