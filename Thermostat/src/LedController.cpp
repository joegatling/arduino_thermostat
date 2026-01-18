#include "LedController.h"

LedController::LedController() : 
    thermostat(nullptr),
    matrix(),
    pixel(1, PIN_NEOPIXEL),

    currentTemperature(0.0f),
    targetTemperature(0.0f),
    useFahrenheit(false),

    lastCurrentTemperatureTime(0),
    lastHeaterModeChangeTime(0),
    lastTargetTemperatureTime(0),
    lastStatusMessageTime(0),

    statusMessage(""),
    statusMessageWidth(0),
    statusMessageHeight(0)
{
    pixel.begin();
    pixel.fill(pixel.Color(255,255,0));
    pixel.show();

    matrix.begin(0x70);  // pass in the address 
    matrix.setFont(&Thermostat_Font);
    matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
    matrix.setTextColor(LED_ON);
    matrix.setTextSize(DISPLAY_SCALE);
    matrix.setRotation(DISPLAY_ROTATION);
    matrix.setBrightness(15);
    matrix.clear();
    
    matrix.setCursor(1 * DISPLAY_SCALE, 5 * DISPLAY_SCALE);
    matrix.print("HI!");  
    
    matrix.writeDisplay();
}

LedController::~LedController()
{
}

void LedController::setThermostat(Thermostat* newThermostat)
{
    if (newThermostat == nullptr)
    {
        return;
    }

    if(thermostat != nullptr)
    {
        return; // Already initialized
    }

    thermostat = newThermostat;

    thermostat->onCurrentTemperatureChanged([this](float newCurrentTemp)
    {
        currentTemperature = newCurrentTemp;
    });

    thermostat->onTargetTemperatureChanged([this](float newTargetTemp)
    {
        targetTemperature = newTargetTemp;
        lastTargetTemperatureTime = millis();
    });

    thermostat->onModeChanged([this](ThermostatMode newMode)
    {        
        lastHeaterModeChangeTime = millis();
        
        previousMode = currentMode;
        currentMode = newMode;
    });

    thermostat->onUseFahrenheitChanged([this](bool newUseFahrenheit)
    {
        useFahrenheit = newUseFahrenheit;
        currentTemperature = thermostat->getCurrentTemperature(); // Update temperature display to reflect unit change
    });

    // Get the current state of the thermostat
    currentTemperature = thermostat->getCurrentTemperature();
    currentMode = thermostat->getMode();
    previousMode = currentMode;
    useFahrenheit = thermostat->isUsingFahrenheit();


}

void LedController::update()
{
    updateMatrix();
    updateNeoPixel();
}

void LedController::showStatusMessage(const String& message)
{
    statusMessage = message;
    lastStatusMessageTime = millis();

    int16_t  x1, y1;
    matrix.getTextBounds(statusMessage, 0, 1, &x1, &y1, &statusMessageWidth, &statusMessageHeight);
}  

bool LedController::shouldShowStatusMessage()
{
    if(lastStatusMessageTime == 0)
    {
        return false;
    }
    else if(statusMessageWidth > 16)
    {
        return getStatusMessageTime() < (unsigned long)(STATUS_MESSAGE_SCROLL_DELAY * 2 + statusMessageWidth * STATUS_MESSAGE_SCROLL_STEP / DISPLAY_SCALE);
    }
    else
    {
        return getStatusMessageTime() < STATUS_MESSAGE_DURATION;
    }    
}

unsigned long LedController::getStatusMessageTime()
{
    return millis() - lastStatusMessageTime;
}

void LedController::drawStatusMessage()
{
  int xOffset = 0;
  
  if(statusMessageWidth > matrix.width())
  {
    if(getStatusMessageTime() > STATUS_MESSAGE_SCROLL_DELAY)
    {
      xOffset = max(-statusMessageWidth+matrix.width(),-(int)(getStatusMessageTime() / STATUS_MESSAGE_SCROLL_STEP * DISPLAY_SCALE));
    } 
  }
  
  matrix.setBrightness(MESSAGE_BRIGHTNESS);
  // Do not multiply xOffset by DISPLAY_SCALE because it already takes into account the scale
  matrix.setCursor(xOffset, 5 * DISPLAY_SCALE);
  matrix.print(statusMessage);
}

void LedController::showProgressBar(float progress)
{
    matrix.setFont(&Thermostat_Font);
    matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
    matrix.setTextSize(DISPLAY_SCALE);
    matrix.setTextColor(LED_ON);

    matrix.clear();

    matrix.setCursor(0, 5 * DISPLAY_SCALE);
    matrix.print(progress * 100);

    int barWidth = progress * matrix.width();
    if(barWidth > 0)
    {
      matrix.fillRect(0,7,barWidth,1,LED_ON);
    }
    matrix.writeDisplay();
}

bool LedController::shouldShowTargetTemperature()
{
    return millis() < lastTargetTemperatureTime + TARGET_TEMP_DURATION;
}

void LedController::drawTargetTemperature()
{
    char str[8];    
    
    if(thermostat == nullptr)
    {
        sprintf(str, "---");
    }
    else if (thermostat->getMode() == OFF)
    {
        sprintf(str, "OFF");
    }
    else
    {
        if(currentMode != OFF && previousMode == OFF && millis() < lastHeaterModeChangeTime + POWER_ON_MSG_DURATION)
        {
            sprintf(str, "ON");
        }
        else
        {
            if (useFahrenheit)
            {
            sprintf(str, "%df", targetTemperature);
            }
            else
            {
            sprintf(str, "%dc", targetTemperature);
            }       
        }
    }

    int x = 1;
    int y = 5;
    int16_t  x1, y1;
    uint16_t w, h;
    
    matrix.setBrightness(TARGET_TEMP_BRIGHTNESS);
    matrix.getTextBounds(str, x * DISPLAY_SCALE, y * DISPLAY_SCALE, &x1, &y1, &w, &h);

    matrix.setCursor(x * DISPLAY_SCALE, y * DISPLAY_SCALE);
    matrix.fillRect((x1-1),(y1-1),(w+2),(h+2),LED_ON);
    matrix.setTextColor(LED_OFF);
    matrix.print(str);   
}

void LedController::drawCurrentTemperature()
{
    char str[8];    
    
    if(thermostat == nullptr)
    {
        sprintf(str, "---");
    }
    else
    {
        if(currentMode == OFF && previousMode != OFF && millis() < lastHeaterModeChangeTime + POWER_ON_MSG_DURATION)
        {
            sprintf(str, "OFF");
        }
        else
        {
            if (useFahrenheit)
            {
            sprintf(str, "%df", currentTemperature);
            }
            else
            {
            sprintf(str, "%dc", currentTemperature);
            }       
        }
    }

    int x = 1;
    int y = 5;
    int16_t  x1, y1;
    uint16_t w, h;
    
    matrix.setBrightness(CURRENT_TEMP_BRIGHTNESS);
    matrix.getTextBounds(str, x * DISPLAY_SCALE, y * DISPLAY_SCALE, &x1, &y1, &w, &h);

    matrix.setCursor(x * DISPLAY_SCALE, y * DISPLAY_SCALE);
    matrix.fillRect((x1-1),(y1-1),(w+2),(h+2),LED_ON);
    matrix.setTextColor(LED_OFF);
    matrix.print(str);   
}

void LedController::updateMatrix()
{
    matrix.clear();

    if(shouldShowStatusMessage())
    {
        drawStatusMessage();
    }
    else if(shouldShowTargetTemperature())
    {  
        drawTargetTemperature();
    }
    else
    {
        drawCurrentTemperature();
    }
    
    matrix.writeDisplay();
}

void LedController::updateNeoPixel()
{
    if(thermostat != nullptr && thermostat->getMode() == BOOST)
    {
        pixel.rainbow((millis()*20) % 0xFFFF);
        pixel.show();
    }
    else
    {
        pixel.clear();
        pixel.show();
    }
}