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
			currentTimestamp = jsonData.current.now;

			maxTargetTemperatureCelsius = jsonData.thermostat.max_temp;
			minTargetTemperatureCelsius = jsonData.thermostat.min_temp;

			$("#loading").hide();
			$(".showAfterLoading").show();					

			
			UpdateTargetTemperatureText();
			UpdateCurrentTemperatureText();

			$("#currentTimestamp").text("Updated " + getTimeDiffString(lastCurrentTemperatureTimestamp, currentTimestamp) + ".");t
		});

		$.getJSON('get-set-history.php', {thermostat: thermostat, count: 6}, function(jsonData) 
		{	
			$("#history").innerHTML = '';	
			
			var historyArray = jsonData.history;

			if(historyArray.length > 1)
			{
				for(var i = hostoryArray.length-2; i >= 0; i--)
				{
					var didTurnOn = historyArray[i].power == true && historyArray[i+1] == false;
					var didTurnOff = historyArray[i].power == false && historyArray[i+1] == true;

					const timestamp = new Date(historyArray[i].timestamp);

					let item = $("#history").createElement("p");
					
					if(didTurnOn)
					{
						
						item.text = timestamp.
					}

				}		
			}	
		});		
	}
}

function getTimeDiffString(timestamp1, timestamp2) 
{
	const date1 = new Date(timestamp1);
	const date2 = new Date(timestamp2);
	const diffMillis = Math.abs(date1.getTime() - date2.getTime());

	console.log("timestamp 1: " + timestamp1);
	console.log("timestamp 2: " + timestamp2);
	console.log(diffMillis);

	if(diffMillis < 1000)
	{
		return "just now";
	}
  
	const timeUnits = [
	  { unit: "second", millis: 1000 },	
	  { unit: "minute", millis: 60 * 1000 },
	  { unit: "hour", millis: 60 * 60 * 1000 },
	  { unit: "day", millis: 24 * 60 * 60 * 1000 },
	  { unit: "week", millis: 7 * 24 * 60 * 60 * 1000 },
	  { unit: "month", millis: 30 * 24 * 60 * 60 * 1000 },
	  { unit: "year", millis: 365 * 24 * 60 * 60 * 1000 },
	];
  
	for (let i = 0; i < timeUnits.length; i++) {
	  const unit = timeUnits[i];
	  const nextUnit = timeUnits[i+1];
	  if (diffMillis < nextUnit.millis) {
		const count = Math.floor(diffMillis / unit.millis);
		return count === 1 ? `${count} ${unit.unit} ago` : `${count} ${unit.unit}s ago`;
	  }
	}
  
	return "unknown time ago";
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

	UpdateCurrentTemperatureText();

	if(currentTemperatureSendTimer != null)
	{
		clearTimeout(currentTemperatureSendTimer);
	}

	currentTemperatureSendTimer = setTimeout(function() { SendTargetTemperatureToServer(); currentTemperatureSendTimer = null;}, temperatureSendTimeout);
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
