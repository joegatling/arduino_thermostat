#include "SimpleButton.h"

#define DEBOUNCE_TIME   20
#define HOLD_TIME  500

SimpleButton::SimpleButton(int pin)
{
  _pin = pin;
  pinMode(pin, INPUT);

  _lastPinReading = LOW;
  _buttonState = LOW;
}

void SimpleButton::Update()
{
  int reading = digitalRead(_pin);

  if(reading != _lastPinReading)
  {
    _debounceTime = millis();
  }
  _lastPinReading = reading;

  if ((millis() - _debounceTime) > DEBOUNCE_TIME) 
  {
      // if the button state has changed:
      if (reading != _buttonState) 
      {
        _buttonState = reading;

        if(_buttonState == HIGH)
        {
          _pressedTime = _debounceTime;

          if(_beginPressFunction)
          {
            _beginPressFunction();
          }
        }
        else
        {
          if(!_didHoldAction)
          {
            // We didn't do a hold this press, so send a click callback
            if(_clickFunction)
            {
              _clickFunction();
            }
          }
          else
          {
            // If we did a hold action, then do nothing here.
            _didHoldAction = false;
          }
        }        
      }
      else
      {
        if(_buttonState == HIGH)
        {
          // Check for hold (If there is a hold action)
          if(_holdFunction && !_didHoldAction)
          {
            if(millis() - _pressedTime > HOLD_TIME)
            {
              _holdFunction();
              _didHoldAction = true;
            }
          }
        }
      }
    }    
}

void SimpleButton::SetClickCallback(callbackFunction function)
{
  _clickFunction = function;
}

void SimpleButton::SetHoldCallback(callbackFunction function)
{
  _holdFunction = function;
}

void SimpleButton::SetBeginPressCallback(callbackFunction function)
{
  _beginPressFunction = function;
}
