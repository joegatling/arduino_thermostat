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
    lastPresetChangeTime(0),

    statusMessage(""),
    statusMessageWidth(0),
    statusMessageHeight(0),
    isQuickMessage(false)
{

}

LedController::~LedController()
{
}

void LedController::initialize()
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
        Serial.print("Current Temperature: ");  
        Serial.println(newCurrentTemp); 
        currentTemperature = newCurrentTemp;
    });

    thermostat->onTargetTemperatureChanged([this](float newTargetTemp)
    {
        Serial.print("Target Temperature: ");  
        Serial.println(newTargetTemp); 
        targetTemperature = newTargetTemp;
        lastTargetTemperatureTime = millis();
    });

    thermostat->onModeChanged([this](ThermostatMode newMode)
    {        
        lastHeaterModeChangeTime = millis();
        
        previousMode = currentMode;
        currentMode = newMode;

        Serial.print("Thermostat mode: ");
        switch(currentMode)
        {
            case OFF: Serial.print("OFF"); break;
            case HEAT: Serial.print("HEAT"); break;
            default: Serial.print("UNKNOWN"); break;
        };
        Serial.print(" (Previous: ");
        switch(previousMode)
        {
            case OFF: Serial.print("OFF"); break;
            case HEAT: Serial.print("HEAT"); break;
            default: Serial.print("UNKNOWN"); break;
        }
        Serial.println(")");

        if(currentMode == OFF)
        {
            showStatusMessage("OFF", true);
        }
        else if(previousMode == OFF)
        {            
            showStatusMessage("ON", true);
        }
    });

    thermostat->onPresetChanged([this](ThermostatPreset newPreset)
    {        
        lastPresetChangeTime = millis();
        
        previousPreset = currentPreset;
        currentPreset = newPreset;

        Serial.print("Thermostat preset: ");
        switch(currentPreset)
        {
            case NONE: Serial.print("NONE"); break;
            case ECO: Serial.print("ECO"); break;
            case BOOST: Serial.print("BOOST"); break;
            case SLEEP: Serial.print("SLEEP"); break;
            default: Serial.print("UNKNOWN"); break;
        };
        Serial.print(" (Previous: ");
        switch(previousPreset)
        {
            case NONE: Serial.print("NONE"); break;
            case ECO: Serial.print("ECO"); break;
            case BOOST: Serial.print("BOOST"); break;
            case SLEEP: Serial.print("SLEEP"); break;
            default: Serial.print("UNKNOWN"); break;
        }
        Serial.println(")");
    });

    thermostat->onUseFahrenheitChanged([this](bool newUseFahrenheit)
    {
        useFahrenheit = newUseFahrenheit;
        currentTemperature = thermostat->getCurrentTemperature(); // Update temperature display to reflect unit change
    });

    // Get the current state of the thermostat
    currentTemperature = thermostat->getCurrentTemperature();
    targetTemperature = thermostat->getTargetTemperature();
    currentMode = thermostat->getMode();
    previousMode = currentMode;
    currentPreset = thermostat->getPreset();
    previousPreset = currentPreset;
    useFahrenheit = thermostat->isUsingFahrenheit();
}

void LedController::update()
{
    updateMatrix();
    updateNeoPixel();
}

void LedController::showStatusMessage(const String& message, bool quickMessage, bool immediate)
{
    statusMessage = message;
    lastStatusMessageTime = millis();
    isQuickMessage = quickMessage && message.length() <= 16;

    int16_t  x1, y1;
    matrix.getTextBounds(statusMessage, 0, 1, &x1, &y1, &statusMessageWidth, &statusMessageHeight);

    if(immediate)
    {
        drawStatusMessage(true);
    }
}  

bool LedController::shouldShowStatusMessage()
{
    if(lastStatusMessageTime == 0 || statusMessage.length() == 0)
    {
        return false;
    }
    else if(statusMessageWidth > 16)
    {
        return getStatusMessageTime() < (unsigned long)(STATUS_MESSAGE_SCROLL_DELAY * 2 + statusMessageWidth * STATUS_MESSAGE_SCROLL_STEP / DISPLAY_SCALE);
    }
    else
    {
        return getStatusMessageTime() < (isQuickMessage ? QUICK_MESSAGE_DURATION : STATUS_MESSAGE_DURATION);
    }    
}

unsigned long LedController::getStatusMessageTime()
{
    return millis() - lastStatusMessageTime;
}

void LedController::drawStatusMessage(bool immediate)
{
  if(immediate)
  {
      matrix.clear();
  }

  int xOffset = 1;
  
  if(statusMessageWidth > matrix.width())
  {
    if(getStatusMessageTime() > STATUS_MESSAGE_SCROLL_DELAY)
    {
        int xMaxScroll = -statusMessageWidth+matrix.width() - 1;
        unsigned long scrollTime = getStatusMessageTime() - STATUS_MESSAGE_SCROLL_DELAY;

        xOffset = max(xMaxScroll, -(int)(scrollTime / STATUS_MESSAGE_SCROLL_STEP * DISPLAY_SCALE + 1));
        //xOffset = max(-statusMessageWidth+matrix.width() - 1,-(int)(getStatusMessageTime() / STATUS_MESSAGE_SCROLL_STEP * DISPLAY_SCALE) + 1);
    } 
  }
  
  matrix.setBrightness(MESSAGE_BRIGHTNESS);
  matrix.setTextColor(LED_ON);
  // Do not multiply xOffset by DISPLAY_SCALE because it already takes into account the scale
  matrix.setCursor(xOffset, 5 * DISPLAY_SCALE);
  matrix.print(statusMessage);

  if(immediate)
  {
      matrix.writeDisplay();
  }
}

void LedController::showProgressBar(float progress)
{
    matrix.setFont(&Thermostat_Font);
    matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
    matrix.setTextSize(DISPLAY_SCALE);
    matrix.setTextColor(LED_ON);

    matrix.clear();

    matrix.setCursor(0, 5 * DISPLAY_SCALE);

    char progressStr[8];
    snprintf(progressStr, sizeof(progressStr), "%.0f", progress * 100);
    matrix.print(progressStr);

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
    else
    {
        if (useFahrenheit)
        {
            snprintf(str, sizeof(str), "%.0ff", targetTemperature);
        }
        else
        {
            snprintf(str, sizeof(str), "%.0fc", targetTemperature);
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
        if (useFahrenheit)
        {
            snprintf(str, sizeof(str), "%.0ff", currentTemperature);
        }
        else
        {
            snprintf(str, sizeof(str), "%.0fc", currentTemperature);
        }               
    }

    int x = 1;
    int y = 5;
    int16_t  x1, y1;
    uint16_t w, h;
    
    matrix.setBrightness(CURRENT_TEMP_BRIGHTNESS);
    matrix.setCursor(x * DISPLAY_SCALE, y * DISPLAY_SCALE);
    matrix.setTextColor(LED_ON);
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
    if(thermostat == nullptr)
    {
        pixel.clear();
        pixel.show();
        return;
    }

    if(thermostat->getMode() == OFF)
    {
        pixel.clear();
        pixel.show();
        return;
    }

    if(thermostat->getPreset() == BOOST)
    {
        pixel.rainbow((millis()*20) % 0xFFFF);
        pixel.show();
        return;
    }

    const long presetFlashTime = 800;
    if(lastPresetChangeTime + presetFlashTime > millis())
    {
        float t = (float)(millis() - lastPresetChangeTime) / presetFlashTime;
        if (t > 1.0f) t = 1.0f;
        if (t < 0.0f) t = 0.0f;

        uint8_t brightness = (uint8_t)((1.0f - t) * 255);
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;

        if (currentPreset == ECO)
        {
            r = 0;
            g = 255;
            b = 0;
        }
        else if (currentPreset == SLEEP)
        {
            r = 0;
            g = 0;
            b = 255;
        }
        else
        {
            r = 64;
            g = 64;
            b = 64;
        }

        // Briefly flash a color based on the current preset (ECO: green, SLEEP: blue, others: gray)
        pixel.fill(pixel.Color(
            (uint8_t)(((uint16_t)r * brightness) / 255),
            (uint8_t)(((uint16_t)g * brightness) / 255),
            (uint8_t)(((uint16_t)b * brightness) / 255)
        ));
        pixel.show();
        return;
    }

    pixel.clear();
    pixel.show();
    return;
}

void LedController::setLightColor(uint8_t r, uint8_t g, uint8_t b)
{
    pixel.fill(pixel.Color(r,g,b));
    pixel.show();
}