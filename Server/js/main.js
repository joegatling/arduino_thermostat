var thermostat = "default";
var currentTemperatureCelsius = 24.0;
var targetTemperatureCelsius = 24.0;
var lastCurrentTemperatureTimestamp = 0;
var lastTargetTemperatureTimestamp = 0;

var maxTargetTemperature = 28.0;
var minTargetTemperature = 18.0;

var temperatureSendTimeout = 1000.0;
var currentTemperatureSendTimer = null;

var apiKey = "fc90b5ba-541b-43a6-a7c0-4c45bf14526d";

function UpdateThermostatData()
{
	$('#loadingInicator').show();
	//$('#infoContainer').load('get-reading.php');

	$.getJSON('get-thermostat-data.php', {key: apiKey}, function(jsonData) 
	{
		//readingData = jsonData.data;

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

		$("#currentTempValue").text(Math.round(Number(currentTemperatureCelsius)));
		$("#setTempValue").text(Math.round(Number(targetTemperatureCelsius)));
		$("#currentTimestamp").text(lastCurrentTemperatureTimestamp);

		$("#loading").hide();
		$(".showAfterLoading").show();					
	});
}

function SendTargetTemperatureToServer()
{
	$.get('set-target-temperature.php', {c:targetTemperatureCelsius, key:apiKey}, function(result)
	{
		currentTemperatureSendTimer = null;
	});

}

function IncreaseSetTemperature()
{
	targetTemperatureCelsius = Math.round(Math.min(targetTemperatureCelsius + 1.0, maxTargetTemperature));
	$("#setTempValue").text(targetTemperatureCelsius);

	if(currentTemperatureSendTimer != null)
	{
		clearTimeout(currentTemperatureSendTimer);
	}

	currentTemperatureSendTimer = setTimeout(function() { SendTargetTemperatureToServer(); currentTemperatureSendTimer = null;}, temperatureSendTimeout);
}

function DecreaseSetTemperature()
{
	targetTemperatureCelsius = Math.round(Math.max(targetTemperatureCelsius - 1.0, minTargetTemperature));
	$("#setTempValue").text(targetTemperatureCelsius);

	if(currentTemperatureSendTimer != null)
	{
		clearTimeout(currentTemperatureSendTimer);
	}

	currentTemperatureSendTimer = setTimeout(function() { SendTargetTemperatureToServer(); currentTemperatureSendTimer = null;}, temperatureSendTimeout);
}
