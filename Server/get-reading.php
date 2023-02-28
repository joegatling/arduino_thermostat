<?php
include_once 'common.php';

$thermostat = "default";
$celsius = 0;
$farenheit = 0;
$timestamp = 0;

if(isset($_GET['thermostat']))
{
	$thermostat = $_GET['thermostat'];
}

$timezoneoffset=0;

$mysqli = new mysqli($dbUrl, $dbUser, $dbPass, $dbName);

if ($mysqli->connect_error) {
	die('Connect Error (' . $mysqli->connect_errno . ') ' . $mysqli->connect_error);
}		

$thermostat = mysqli_real_escape_string($mysqli, $thermostat);


$thermostatInfo = GetThermostatInfo($thermostat);

$timezone = $thermostatInfo['time_zone'];



$currentTemp = 0;

$query = "SET time_zone = '$timezone';";	
$query .= "SELECT timestamp, celsius FROM $tableCurrentTemperature ORDER BY timestamp DESC LIMIT 1;";

if($result = $mysqli->query($query))
{
	while($row = $result->fetch_assoc())
	{
		$celsius = $row['celsius'];	
		$timestamp = $row['timestamp'];	
		
		$farenheit = ($celsius * 9) / 5 + 32;
	}
	
	$result->free();
}
else
{
	echo("<p>No Data</p>");
}

$mysqli->close(); 
?>		

<div id="infoCard">
	<p class="tempInfo"><span id="temp"><?php echo(round($celsius)); ?></span>C</p>
	<p class="altTempInfo"><span id="altTemp"><?php echo(round($farenheit)); ?></span>F</p>
    <p class="dateInfo"><span id="date">
    <?php 
    	$currentTime = time();
    	$timeOffset = $currentTime - $timestamp;
    	
    	$minutes = floor($timeOffset / 60);
    	$hours = floor($timeOffset / 3600);
    	$days = floor($timeOffset / 86400);
    	
    	if($minutes == 0)
    	{
    		if($timeOffset == 1)
    		{
    			echo($timeOffset . " second ago.");
    		}
    		else
    		{
    	    	echo($timeOffset . " seconds ago.");	
    		}
    	}
    	else if($hours == 0)
    	{
    		if($minutes == 1)
    		{
    			echo($minutes . " minute ago.");
    		}
    		else
    		{
    	    	echo($minutes . " minutes ago.");	
    		}
    	}
    	else if($days == 0)
    	{
    		if($hours == 1)
    		{
    			echo($hours . " hour ago.");
    		}
    		else
    		{
    	    	echo($hours . " hours ago.");	
    		}    	
    	}
    	else if($days <= 10)
    	{
    		if($days == 1)
    		{
    			echo($days . " day ago.");
    		}
    		else
    		{
    	    	echo($days . " days3 ago.");	
    		}  
    			
    	}
    	else
    	{
    		echo(date("G:i j M",$timestamp));     	
		}    
    ?>
    </span></p>
	<div id="controls">
	<a href="#" alt="Decrease Temperature">-</a>
	<a href="#" alt="Increase Temperature">+</a>
	</div>
	<p hidden id="loadingInicator">Loading...</p>    
</div>