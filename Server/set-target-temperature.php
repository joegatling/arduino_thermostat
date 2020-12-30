<?php
include_once 'common.php';

$user = "defaultUser";

$thermostat = $defaultThermostat;
$celsius = "-270";
$result = "OK";

$isThermostatOn = true;

$minTemperature = 18.0;
$maxTemperature = 28.0;

$api_key = "";
// Process all passed arguments

if(isset($_GET['c']))
{
	$celsius = $_GET['c'];
}
else if(isset($_GET['f']))
{
	$celsius = ($_GET['f'] - 32) * 5/9;
}
else
{
	$result = "ERR_NO_TEMP";
}

if(isset($_GET['key']))
{
	$api_key = $_GET['key'];
}
else
{
	$result = "ERR_NO_KEY";
}

if(isset($_GET['user']))
{
	$user = $_GET['user'];
}

if(isset($_GET['power']))
{
	$isThermostatOn = $_GET['power'];
}

if(isset($_GET['thermostat']))
{
	$thermostat = $_GET['thermostat'];
}

if($result == "OK")
{
	$celsius = max($minTemperature, min($maxTemperature, $celsius));

	$privileges = GetPrivileges($api_key);

	$thermostatInfo = GetThermostatInfo($thermostat);
	$timestamp = new DateTime("now", new DateTimeZone($thermostatInfo['time_zone']));
	$formattedTimestamp = $timestamp->format('Y-m-d H:i:s');


	if($privileges->CanWriteTarget())
	{
		$mysqli = new mysqli($dbUrl, $dbUser, $dbPass, $dbName);
		
		if ($mysqli->connect_error) {
			die('Connect Error (' . $mysqli->connect_errno . ') ' . $mysqli->connect_error);
		}		

		$timestamp = time();
		$query = "INSERT INTO $tableTargetTemperature (thermostat, user, celsius, power, timestamp) VALUES('$thermostat', '$user', $celsius, $isThermostatOn, '$formattedTimestamp')";
		$mysqli->query($query);

		$mysqli->close();

		$result = "SUCCESS";
	}
	else
	{
		$result = "ERR_INVALID_PRIVILEGES";
	}
}

echo($result);
?>

