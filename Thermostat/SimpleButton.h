#ifndef SIMPLE_BUTTON_H
#define SIMPLE_BUTTON_H

#include <Arduino.h>

extern "C" {
typedef void (*callbackFunction)(void);
}


class SimpleButton
{
  public:
    SimpleButton(int pin);

    void Update();

    void SetBeginPressCallback(callbackFunction function);
    void SetClickCallback(callbackFunction function);
    void SetHoldCallback(callbackFunction function);

  private:
    int _pin;

    int _buttonState;
    int _lastPinReading;

    unsigned long _debounceTime;
    unsigned long _pressedTime;   

    bool _didHoldAction = false;

    callbackFunction _beginPressFunction = NULL;
    callbackFunction _clickFunction = NULL;
    callbackFunction _holdFunction = NULL;
    
};

#endif // SIMPLE_BUTTON_H
