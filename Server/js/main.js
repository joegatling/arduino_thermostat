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

var chart = null;

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

			$("#currentTimestamp").text("Updated " + getTimeDiffString(lastCurrentTemperatureTimestamp, currentTimestamp) + ".");
		});

		// $.getJSON('get-set-history.php', {thermostat: thermostat, count: 6}, function(jsonData) 
		// {	
		// 	$("#history").innerHTML = '';	
			
		// 	var historyArray = jsonData.history;

		// 	if(historyArray.length > 1)
		// 	{
		// 		for(var i = hostoryArray.length-2; i >= 0; i--)
		// 		{
		// 			var didTurnOn = historyArray[i].power == true && historyArray[i+1] == false;
		// 			var didTurnOff = historyArray[i].power == false && historyArray[i+1] == true;

		// 			const timestamp = new Date(historyArray[i].timestamp);

		// 			let item = $("#history").createElement("p");
					
		// 			if(didTurnOn)
		// 			{
						
		// 				item.text = timestamp.
		// 			}

		// 		}		
		// 	}	
		// });		
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

let width, height, gradient, setGradient
function getGradient(ctx, chartArea) {
	const chartWidth = chartArea.right - chartArea.left;
	const chartHeight = chartArea.bottom - chartArea.top;
	if (!gradient || width !== chartWidth || height !== chartHeight) {
	  // Create the gradient because this is either the first render
	  // or the size of the chart has changed
	  width = chartWidth;
	  height = chartHeight;
	  gradient = ctx.createLinearGradient(chartArea.left, 0, chartArea.right, 0);
	  gradient.addColorStop(1, '#ffefaa');
	  gradient.addColorStop(0.9, '#e8b200');
	  gradient.addColorStop(0.2, '#e8b200');
	  gradient.addColorStop(0, '#d18e00');
	}
  
	return gradient;
  }

  function getSetGradient(ctx, chartArea) {
	const chartWidth = chartArea.right - chartArea.left;
	const chartHeight = chartArea.bottom - chartArea.top;
	if (!setGradient || width !== chartWidth || height !== chartHeight) {
	  // Create the gradient because this is either the first render
	  // or the size of the chart has changed
	  width = chartWidth;
	  height = chartHeight;
	  setGradient = ctx.createLinearGradient(chartArea.left, 0, chartArea.right, 0);
	  setGradient.addColorStop(1, '#b47c00');
	  setGradient.addColorStop(0.5, '#b47c00');
	  setGradient.addColorStop(0, '#d18e00');
	}
  
	return setGradient;
  }  

function DrawChart() 
{
    const queryString = window.location.search;

    // Create a URLSearchParams object
    const params = new URLSearchParams(queryString);

    var interval = 12;
    // Access individual query parameters
    // Check if a parameter exists
    if (params.has("interval")) {
        interval = params.get("interval");
    }

    // Iterate through all parameters
    for (const [key, value] of params) {
        console.log(`${key}: ${value}`);
    }

    $.ajax({
        url: "get-data-new.php",
        dataType: "json",
        // async: false,
        data: {
            interval: interval,
			unit: (useFahrenheit ? "f" : "c")
        },
		success: function(jsonData) {
			//var jsonData = JSON.parse(data);

			if(chart != null)
			{
				chart.data.labels = jsonData.labels;
				chart.data.datasets[0].data = jsonData.temps;
				chart.data.datasets[1].data = jsonData.sets;
				chart.update();

				console.log("Updated chart data.");
			}
			else
			{
				const ctx = document.getElementById('temperatureChart').getContext('2d');

				const devicePixelRatio = window.devicePixelRatio || 1;
				ctx.scale(devicePixelRatio, devicePixelRatio);

				chart = new Chart(ctx, {
				type: 'line',
					data: {
						labels: jsonData.labels,
						datasets: [{
						label: 'Temperature',
						data: jsonData.temps,
						borderWidth: 3 / window.devicePixelRatio,
						tension: 0.2,
						pointStyle: 'circle',
						pointRadius: jsonData.temps.map((_, index) => 
							index === jsonData.temps.length - 1 ? 4 / window.devicePixelRatio : 0 // Only the last point is visible
						),
						pointBackgroundColor: '#ffefaa',
						borderCapStyle: 'round',
						//borderColor: '#ffd600'
						borderColor: function(context) {
							const chart = context.chart;
							const {ctx, chartArea} = chart;
					
							if (!chartArea) {
							  // This case happens on initial chart load
							  return;
							}
							return getGradient(ctx, chartArea);
						  }
		
					},{
						label: 'Set',
						data: jsonData.sets,
						borderWidth: 3  / window.devicePixelRatio,
						tension: 0.0,
						pointStyle: false,
						borderDash: [4, 4],
						//borderColor: '#423100'
						borderColor: function(context) {
							const chart = context.chart;
							const {ctx, chartArea} = chart;
					
							if (!chartArea) {
							  // This case happens on initial chart load
							  return;
							}
							return getSetGradient(ctx, chartArea);
						  }
					}]
				},
				options: {
					animation: false,
					plugins: {
					legend: {
						display: false
					}                
					},
					scales: {
					x: {
						display: false,
					},
					y: {
						display: false,
					}
					},
					layout: {
						padding: {
							right: 20
						}
					}					
				},
				plugins: [
					{
						id: 'finalValuePlugin',
						afterDatasetDraw(chart) {
							const { ctx, chartArea, data } = chart;
							const dataset = data.datasets[0]; // Assume single dataset
							const meta = chart.getDatasetMeta(0); // Meta for the first dataset
			
							// Find the final point
							const lastIndex = dataset.data.length - 1;
							const lastPoint = meta.data[lastIndex]; // Element for the last point
							const value = Math.round(dataset.data[lastIndex]); // Value of the last data point

							const dpr = window.devicePixelRatio || 1; // Get the device pixel ratio
							const fontSize = 8 * dpr; 							
			
							if (lastPoint) {
								ctx.save();
								ctx.font = '${fontSize}px Roboto'; // Customize font size and style
								ctx.fillStyle = '#ffefaa'; // Match point color
								ctx.textAlign = 'left';

								ctx.fillText(value, lastPoint.x + 8, lastPoint.y + 4); // Position above the final point
								ctx.restore();
							}
						}
					}
				]});
				
				const chartElement = document.getElementById('temperatureChart');
				setTimeout(() => {
					chartElement.style.opacity = 1; // Make the element fully visible
				}, 1);
			}
		},
		error: function(xhr, status, error) {
			console.error("Error: ", status, error);
		}
	});



        
}
