var thermostat = "beachwood";

var currentTemperature = 24.0;
var targetTemperature = 24.0;

var lastCurrentTemperatureTimestamp = 0;
var lastTargetTemperatureTimestamp = 0;

var isThermostatOn = true;
var useFahrenheit = true;

var maxTargetTemperatureCelsius = 30.0;
var minTargetTemperatureCelsius = 10.0;

var temperatureSendTimeout = 1000.0;
var currentTemperatureSendTimer = null;

var currentMessageTimeout = null;

var apiKey = "fc90b5ba-541b-43a6-a7c0-4c45bf14526d";

function UpdateThermostatData()
{
	if(currentTemperatureSendTimer == null)
	{
		$('#loadingInicator').show();
		//$('#infoContainer').load('get-reading.php');

		$.getJSON('get-thermostat-data.php', {key: apiKey}, function(jsonData) 
		{						
			if(currentTemperatureSendTimer == null)
			{
				if(IsTemperatureFahrenheit())
				{
					currentTemperature = parseFloat(jsonData.current.farenheit);
				}
				else
				{
					currentTemperature = parseFloat(jsonData.current.celsius);
				}

				lastCurrentTemperatureTimestamp = jsonData.current.timestamp;	
			}
			else
			{
				console.log("Rejecting temperature from server because we just set it locally.");
			}

			if(IsTemperatureFahrenheit())
			{
				targetTemperature = parseFloat(jsonData.target.farenheit);
			}
			else
			{
				targetTemperature = parseFloat(jsonData.target.celsius);
			}

			lastTargetTemperatureTimestamp = jsonData.target.timestamp;
			isThermostatOn = jsonData.target.power > 0;

			maxTargetTemperatureCelsius = jsonData.thermostat.max_temp;
			minTargetTemperatureCelsius = jsonData.thermostat.min_temp;

			$("#loading").hide();
			$(".showAfterLoading").show();					

			
			UpdateTargetTemperatureText();
			UpdateCurrentTemperatureText();

      HideMessage();

			$("#currentTimestamp").text(lastCurrentTemperatureTimestamp);
		});
	}
}

function UpdateTargetTemperatureText()
{
	if(IsTemperatureFahrenheit())
	{
		$("#setTempValue").text(Math.round(Number(targetTemperature)));
		$("#setTempSymbol").text("F");

	}
	else
	{
		$("#setTempValue").text(Math.round(Number(targetTemperature)));
		$("#setTempSymbol").text("C");
	}
}

function UpdateCurrentTemperatureText()
{
	if(IsTemperatureFahrenheit())
	{
		$("#currentTempValue").text(Math.round(Number(currentTemperature)));
		$("#currentTempSymbol").text("F");
	}
	else
	{
		$("#currentTempValue").text(Math.round(Number(currentTemperature)));
		$("#currentTempSymbol").text("C");
	}
}

function SendTargetTemperatureToServer()
{
	if(IsTemperatureFahrenheit())
	{
		$.get('set-target-temperature.php', {f:targetTemperature, key:apiKey, thermostat:thermostat, power:(isThermostatOn ? 1 : 0)}, function(result)
		{
			currentTemperatureSendTimer = null;
		});
	}
	else
	{
		$.get('set-target-temperature.php', {c:targetTemperature, key:apiKey, thermostat:thermostat, power:(isThermostatOn ? 1 : 0)}, function(result)
		{
			currentTemperatureSendTimer = null;
		});
	
	}

}

function CelsiusToFahrenheit(celsius)
{
	return celsius * 9/5 + 32;
}

function FahrenheitToCelsius(fahrenheit)
{
	return (fahrenheit - 32) * 5/9;
}


function IncreaseSetTemperature()
{
	var newTemp = targetTemperature + 1;

	// if(IsTemperatureFarenheit())
	// {
	// 	var f = CelsiusToFahrenheit(targetTemperature);
	// 	newTemp = FahrenheitToCelsius(f + 1);
	// }

	maxTemp = maxTargetTemperatureCelsius;
	if(useFahrenheit)
	{
		maxTemp = CelsiusToFahrenheit(maxTargetTemperatureCelsius);		
	}

	targetTemperature = Math.round(Math.min(newTemp, maxTemp));
	
	UpdateTargetTemperatureText();

	if(currentTemperatureSendTimer != null)
	{
		clearTimeout(currentTemperatureSendTimer);
	}

	currentTemperatureSendTimer = setTimeout(function() { SendTargetTemperatureToServer(); currentTemperatureSendTimer = null;}, temperatureSendTimeout);
}

function DecreaseSetTemperature()
{
	var newTemp = targetTemperature - 1;

	// if(IsTemperatureFarenheit())
	// {
	// 	var f = CelsiusToFahrenheit(targetTemperature);
	// 	newTemp = FahrenheitToCelsius(f - 1);
	// }


	minTemp = minTargetTemperatureCelsius;
	if(useFahrenheit)
	{
		minTemp = CelsiusToFahrenheit(minTargetTemperatureCelsius);		
	}

	targetTemperature = Math.round(Math.max(newTemp, minTemp));

	UpdateTargetTemperatureText();

	if(currentTemperatureSendTimer != null)
	{
		clearTimeout(currentTemperatureSendTimer);
	}

	currentTemperatureSendTimer = setTimeout(function() { SendTargetTemperatureToServer(); currentTemperatureSendTimer = null;}, temperatureSendTimeout);
}

function TogglePower()
{
	isThermostatOn = !isThermostatOn;

  if(isThermostatOn)
  {
    ShowMessage("ON");
  }
  else
  {
    ShowMessage("OFF");
  }

	UpdateCurrentTemperatureText();

	if(currentTemperatureSendTimer != null)
	{
		clearTimeout(currentTemperatureSendTimer);
	}

	currentTemperatureSendTimer = setTimeout(function() { SendTargetTemperatureToServer(); currentTemperatureSendTimer = null;}, temperatureSendTimeout);
}

function ShowMessage(message)
{
  $("#message").text(message);

  $("#setTemp").hide();
  $("#currentTemp").hide();
  $("#message").show();

  if(currentMessageTimeout != null)
  {
    clearTimeout(currentMessageTimeout);
  }

  currentMessageTimeout = setTimeout(function() { HideMessage(); }, 1000);
}

function HideMessage()
{
	if(isThermostatOn == true)
	{
		$("#setTemp").show();
    $("#currentTemp").hide();
	}
	else
	{
		$("#setTemp").hide();
    $("#currentTemp").show();
	}	

  $("#message").hide();

  currentMessageTimeout = null;
}

function SetTemperatureToFahrenheit(f)
{
	if(useFahrenheit != f)
	{
		if(f == true)
		{
			currentTemperature = CelsiusToFahrenheit(currentTemperature);
			targetTemperature = CelsiusToFahrenheit(targetTemperature);
		}
		else
		{
			currentTemperature = FahrenheitToCelsius(currentTemperature);
			targetTemperature = FahrenheitToCelsius(targetTemperature);
		}

		useFahrenheit = f;
		document.cookie = "fahrenheit=" + (f ? 1 : 0);
	
	}

	UpdateTargetTemperatureText();
	UpdateCurrentTemperatureText();

	if(f)
	{
		$("#fahrenheitToggle").hide();
		$("#celsiusToggle").show();
	}
	else
	{
		$("#fahrenheitToggle").show();
		$("#celsiusToggle").hide();
	}

}

function IsTemperatureFahrenheit()
{
	return useFahrenheit;


	// return false;	
}

function ReadCookieData()
{
	if (navigator.cookieEnabled)
	{
		var allCookies = document.cookie;
				
		cookieArray = allCookies.split(';');
		
		// Now take key value pair out of this array
		for(var i=0; i<cookieArray.length; i++) {
		name = cookieArray[i].split('=')[0];
		value = cookieArray[i].split('=')[1];
		
		if(name == "fahrenheit")
		{
			useFahrenheit = Number(value) == 1;
		}
		}	
	}
}
