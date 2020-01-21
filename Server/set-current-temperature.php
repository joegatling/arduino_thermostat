<?php
include_once 'common.php';

// $dbUser = "nodemcu";
// $dbPass = "netxaX-zabtej-6qucmy";
// $dbUrl = "joegatling.powwebmysql.com";
// $dbName = "joegatling_thermostat";
// $table = "temperature_read";

$thermostat = $defaultThermostat;

$celsius = "-270";
$result = "OK";

$api_key = "";

if(isset($_GET['c']))
{
	$celsius = $_GET['c'];
}
else if(isset($_GET['f']))
{
	$celsius = $_GET['f'] * 100/180 + 32;
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

if(isset($_GET['thermostat']))
{
	$thermostat = $_GET['thermostat'];
}



if($result == "OK")
{
	$thermostatInfo = GetThermostatInfo($thermostat);
	$timestamp = new DateTime("now", new DateTimeZone($thermostatInfo['time_zone']));
	$formattedTimestamp = $timestamp->format('Y-m-d H:i:s');

	$privileges = GetPrivileges($api_key);

	if($privileges->CanWriteCurrent())
	{
		$mysqli = new mysqli($dbUrl, $dbUser, $dbPass, $dbName);

		$query = "INSERT INTO $tableCurrentTemperature (thermostat, celsius, timestamp) VALUES('$thermostat', $celsius, '$formattedTimestamp')";

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

