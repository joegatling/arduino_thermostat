<!DOCTYPE html>

<?php
include_once 'config.php';

if(isset($_GET['zone']))
{
	$zoneId = $_GET['zone'];
}
?>

<!--[if lt IE 7]>      <html class="no-js lt-ie9 lt-ie8 lt-ie7"> <![endif]-->
<!--[if IE 7]>         <html class="no-js lt-ie9 lt-ie8"> <![endif]-->
<!--[if IE 8]>         <html class="no-js lt-ie9"> <![endif]-->
<!--[if gt IE 8]><!--> <html class="no-js"> <!--<![endif]-->
    <head>
        <meta charset="utf-8">
        <meta http-equiv="X-UA-Compatible" content="IE=edge,chrome=1">
        <title>Temperature</title>
        <meta name="description" content="Temperature Monitor">
        <meta name="viewport" content="width=device-width">

        <link rel="stylesheet" href="css/normalize.min.css">
        <link rel="stylesheet" href="css/main.css">

        <link href="https://fonts.googleapis.com/css?family=Roboto:100,200,400,900&display=swap" rel="stylesheet">

        <script src="js/vendor/modernizr-2.6.2-respond-1.1.0.min.js"></script>
    </head>
    <body>
        <!--[if lt IE 7]>
            <p class="chromeframe">You are using an <strong>outdated</strong> browser. Please <a href="http://browsehappy.com/">upgrade your browser</a> or <a href="http://www.google.com/chromeframe/?redirect=true">activate Google Chrome Frame</a> to improve your experience.</p>
        <![endif]-->
        
        <div id="container">
            <div id="square">
                <div id="content">
                    <div id="infoCard">                        
                        <p id="loading">Loading...</p>
                        <p class="initiallyHidden message" id="message">HELLO</p>
                        <p class="setTemp showAfterLoading" id="setTemp"><span id="setTempValue">24</span><span id="setTempSymbol">C</span></p>
                        <p class="currentTemp showAfterLoading" id="currentTemp"><span id="currentTempValue">24</span><span id="currentTempSymbol">C</span></p>
                        <!-- <p class="currentTempAlt showAfterLoading"><span id="currentTempAltValue">75</span>F</p> -->
                    </div>
                    <a id="togglePower" class="controlButton" href="#" alt="Power"><span>Power</span></a>
                    <a id="decreaseTemp" class="controlButton" href="#" alt="Decrease Temperature"><span>-</span></a>
                    <a id="increaseTemp" class="controlButton" href="#" alt="Increase Temperature"><span>+</span></a>
                    <div id="optionsCard">
                        <p class="currentTimestamp showAfterLoading" id="currentTimestamp">Updated moments ago</p>
                        <p class="formatToggle showAfterLoading" id="temperatureFormat">
                            <a id="celsiusToggle" href="">Use Celsius</a>
                            <a id="fahrenheitToggle" href="">Use Fahrenheit</a>
                        </p>
                    </div>
                </div>
            </div>
        </div>

        <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js"></script>
        <script>window.jQuery || document.write('<script src="js/vendor/jquery-1.10.1.min.js"><\/script>')</script>

        <script src="js/main.js"></script>
        
        <script>
            $("#loading").show();
            $(".showAfterLoading").hide();

            ReadCookieData();
            UpdateThermostatData();
        	
            $(document).ready(function () 
            { 
                setInterval(UpdateThermostatData, 5000);
                
                $("#increaseTemp").click(function(e) {e.preventDefault(); IncreaseSetTemperature(); return false; });
                $("#decreaseTemp").click(function(e) {e.preventDefault(); DecreaseSetTemperature(); return false; });
                $("#togglePower").click(function(e) {e.preventDefault(); TogglePower(); return false; });

                if(IsTemperatureFahrenheit())
                {
                    $("#fahrenheitToggle").hide();
                    $("#celsiusToggle").show();
                }
                else
                {
                    $("#fahrenheitToggle").show();
                    $("#celsiusToggle").hide();
                }

                $("#fahrenheitToggle").click(function(e) {e.preventDefault(); SetTemperatureToFahrenheit(true); return false; });
                $("#celsiusToggle").click(function(e) {e.preventDefault(); SetTemperatureToFahrenheit(false); return false; });
			});        	
        </script>


        <script>
            var _gaq=[['_setAccount','UA-XXXXX-X'],['_trackPageview']];
            (function(d,t){var g=d.createElement(t),s=d.getElementsByTagName(t)[0];
            g.src='//www.google-analytics.com/ga.js';
            s.parentNode.insertBefore(g,s)}(document,'script'));
        </script>
    </body>
</html>
