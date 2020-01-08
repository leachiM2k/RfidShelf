/**
 * EasyButtonMCP.cpp
 * @author Evert Arias
 * @version 1.1.0
 * @license MIT
 */

#include "EasyButtonMCP.h"

void EasyButtonMCP::begin()
{
    _mcp.begin();
	_mcp.pinMode(_pin, _pu_enabled ? INPUT_PULLUP : INPUT);
	_current_state = _readPin();
	if (_invert) _current_state = !_current_state;
	_time = millis();
	_last_state = _current_state;
	_changed = false;
	_last_change = _time;
}

void EasyButtonMCP::onPressed(EasyButtonMCP::callback_t callback)
{
	_pressed_callback = callback;
}

void EasyButtonMCP::onPressedFor(uint32_t duration, EasyButtonMCP::callback_t callback)
{
	_held_threshold = duration;
	_pressed_for_callback = callback;
}

void EasyButtonMCP::onSequence(uint8_t sequences, uint32_t duration, EasyButtonMCP::callback_t callback)
{
	_press_sequences = sequences;
	_press_sequence_duration = duration;
	_pressed_sequence_callback = callback;
}

bool EasyButtonMCP::isPressed()
{
	return _current_state;
}

bool EasyButtonMCP::isReleased()
{
	return !_current_state;
}

bool EasyButtonMCP::wasPressed()
{
	return _current_state && _changed;
}

bool EasyButtonMCP::wasReleased()
{
	return !_current_state && _changed;
}

bool EasyButtonMCP::pressedFor(uint32_t duration)
{
	return _current_state && _time - _last_change >= duration;
}

bool EasyButtonMCP::releasedFor(uint32_t duration)
{
	return !_current_state && _time - _last_change >= duration;
}

bool EasyButtonMCP::read(int read_type)
{
	uint32_t read_started_ms = millis();

	bool pinVal = _readPin();

	if (_invert)
		pinVal = !pinVal;

	if (read_started_ms - _last_change < _db_time)
	{ 	//true -> debounce time has not ellapsed
		_changed = false;
	}
	else
	{	//true -> debounce time ellapsed 
		_last_state = _current_state;				// save last state.
		_current_state = pinVal;					// assign new state as current state from pin's value.
		_changed = (_current_state != _last_state); // report state change if current state vary from last state.
		// if state has changed since last read.
		if (_changed)
		{ // state change
			// save current millis as last change time.
			_last_change = read_started_ms;
		}
	}

	if (wasReleased())
	{
		if (!_was_btn_held)
		{
			if (_short_press_count == 0)
				_first_press_time = read_started_ms;

			_short_press_count++;

			if (_pressed_callback)
				_pressed_callback();

			if (_short_press_count == _press_sequences && _press_sequence_duration >= (read_started_ms - _first_press_time))
			{ //true-> pressed_sequence
				if (_pressed_sequence_callback)
				{
					_pressed_sequence_callback();
				}
				_short_press_count = 0;
				_first_press_time = 0; //MOdificar
			}

			else if (_press_sequence_duration <= (read_started_ms - _first_press_time))
			{ // true-> sequence timeout
				_short_press_count = 0;
				_first_press_time = 0;
			}
		}
		// button was not held.
		else
		{
			_was_btn_held = false;
		}
		// since button released, reset _pressed_for_callbackCalled value.
		_held_callback_called = false;
	}
	else if(isPressed() && read_type == POLL)
		_checkPressedTime();
	

	_time = read_started_ms;

	return _current_state;
}

bool EasyButtonMCP::_readPin()
{
	return _mcp.digitalRead(_pin);
}

bool EasyButtonMCP::supportsInterrupt()
{
	return (digitalPinToInterrupt(_pin) != NOT_AN_INTERRUPT);
}

void EasyButtonMCP::enableInterrupt(EasyButtonMCP::callback_t callback)
{
	attachInterrupt(digitalPinToInterrupt(_pin), callback, CHANGE);
}

void EasyButtonMCP::disableInterrupt()
{
	detachInterrupt(digitalPinToInterrupt(_pin));
}

void EasyButtonMCP::update()
{
	if (!_was_btn_held)
		_checkPressedTime();
}

void EasyButtonMCP::_checkPressedTime()
{
	uint32_t read_started_ms = millis();
	if (_current_state && read_started_ms - _last_change >= _held_threshold && _pressed_for_callback)
	{
		// button has been pressed for at least the given time
		_was_btn_held = true;
		// reset short presses counters.
		_short_press_count = 0;
		_first_press_time = 0;
		// call the callback function for a long press event if it exist and if it has not been called yet.
		if (_pressed_for_callback && !_held_callback_called)
		{
			_held_callback_called = true; // set as called.
			_pressed_for_callback();
		}
	}
}