<?php
$dbUser = "nodemcu";
$dbPass = "netxaX-zabtej-6qucmy";
$dbUrl = "joegatling.powwebmysql.com";
$dbName = "joegatling_thermostat";
$table = "temperature_set";

include_once 'common.php';

$user = "defaultUser";

$thermostat = "default";
$celsius = "-270";
$result = "OK";

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

if(isset($_GET['user']))
{
	$user = $_GET['user'];
}

if(isset($_GET['thermostat']))
{
	$thermostat = $_GET['thermostat'];
}

if($result == "OK")
{
	$privileges = GetPrivileges($api_key);

	if($privileges->CanWriteTarget())
	{
		$mysqli = new mysqli($dbUrl, $dbUser, $dbPass, $dbName);
		
		if ($mysqli->connect_error) {
			die('Connect Error (' . $mysqli->connect_errno . ') ' . $mysqli->connect_error);
		}		

		$timestamp = time();
		$query = "INSERT INTO $tableTargetTemperature (thermostat, user, celsius) VALUES('$thermostat', '$user', $celsius)";
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

