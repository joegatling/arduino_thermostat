var thermostat = "beachwood";
var currentTemperatureCelsius = 24.0;
var targetTemperatureCelsius = 24.0;
var lastCurrentTemperatureTimestamp = 0;
var lastTargetTemperatureTimestamp = 0;

var isThermostatOn = true;

var maxTargetTemperature = 28.0;
var minTargetTemperature = 10.0;

var temperatureSendTimeout = 1000.0;
var currentTemperatureSendTimer = null;

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
				currentTemperatureCelsius = parseFloat(jsonData.current.celsius);
				lastCurrentTemperatureTimestamp = jsonData.current.timestamp;
			}
			else
			{
				console.log("Rejecting temperature from server because we just set it locally.");
			}

			targetTemperatureCelsius = parseFloat(jsonData.target.celsius);
			lastTargetTemperatureTimestamp = jsonData.target.timestamp;
			isThermostatOn = jsonData.target.power > 0;

			$("#loading").hide();
			$(".showAfterLoading").show();					

			
			UpdateTargetTemperatureText();
			UpdateCurrentTemperatureText();

			$("#currentTimestamp").text(lastCurrentTemperatureTimestamp);
		});
	}
}

function UpdateTargetTemperatureText()
{
	if(IsTemperatureFarenheit())
	{
		$("#setTempValue").text(Math.round(CelsiusToFahrenheit(Number(targetTemperatureCelsius))));
	}
	else
	{
		$("#setTempValue").text(Math.round(Number(targetTemperatureCelsius)));
	}
}

function UpdateCurrentTemperatureText()
{
	if(IsTemperatureFarenheit())
	{
		$("#currentTempValue").text(Math.round(CelsiusToFahrenheit(Number(currentTemperatureCelsius))));
	}
	else
	{
		$("#currentTempValue").text(Math.round(Number(currentTemperatureCelsius)));
	}

	if(isThermostatOn == true)
	{
		$("#setTemp").show();
		$("#heaterOff").hide();
	}
	else
	{
		$("#setTemp").hide();
		$("#heaterOff").show();
	}	
}

function SendTargetTemperatureToServer()
{
	$.get('set-target-temperature.php', {c:targetTemperatureCelsius, key:apiKey, thermostat:thermostat, power:(isThermostatOn ? 1 : 0)}, function(result)
	{
		currentTemperatureSendTimer = null;
	});

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
	var newTemp = targetTemperatureCelsius + 1;

	if(IsTemperatureFarenheit())
	{
		var f = CelsiusToFahrenheit(targetTemperatureCelsius);
		newTemp = FahrenheitToCelsius(f + 1);
	}

	targetTemperatureCelsius = Math.round(Math.min(newTemp, maxTargetTemperature));
	
	UpdateTargetTemperatureText();

	if(currentTemperatureSendTimer != null)
	{
		clearTimeout(currentTemperatureSendTimer);
	}

	currentTemperatureSendTimer = setTimeout(function() { SendTargetTemperatureToServer(); currentTemperatureSendTimer = null;}, temperatureSendTimeout);
}

function DecreaseSetTemperature()
{
	var newTemp = targetTemperatureCelsius - 1;

	if(IsTemperatureFarenheit())
	{
		var f = CelsiusToFahrenheit(targetTemperatureCelsius);
		newTemp = FahrenheitToCelsius(f - 1);
	}

	targetTemperatureCelsius = Math.round(Math.max(newTemp, minTargetTemperature));

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

	UpdateCurrentTemperatureText();

	if(currentTemperatureSendTimer != null)
	{
		clearTimeout(currentTemperatureSendTimer);
	}

	currentTemperatureSendTimer = setTimeout(function() { SendTargetTemperatureToServer(); currentTemperatureSendTimer = null;}, temperatureSendTimeout);
}

function SetTemperatureToFahrenheit(isFahrenheit)
{
	document.cookie = "fahrenheit=" + (isFahrenheit ? 1 : 0);

	UpdateTargetTemperatureText();
	UpdateCurrentTemperatureText();

	if(isFahrenheit)
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

function IsTemperatureFarenheit()
{
	var allCookies = document.cookie;
			
	cookieArray = allCookies.split(';');
	   
	// Now take key value pair out of this array
	for(var i=0; i<cookieArray.length; i++) {
	   name = cookieArray[i].split('=')[0];
	   value = cookieArray[i].split('=')[1];
	   
	   if(name == "fahrenheit")
	   {
		   return Number(value) == 1;
	   }
	}	
	
	return false;	
}