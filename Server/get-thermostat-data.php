<?php
include_once 'common.php';

$errorMessage = "";

$thermostat = $defaultThermostat;

if(isset($_GET['thermostat']))
{
	$thermostat = $_GET['thermostat'];
}

$api_key = "";
if(isset($_GET['key']))
{
	$api_key = $_GET['key'];
}
else
{
	$errorMessage = "ERR_NO_KEY";
}

$privileges = GetPrivileges($api_key);
$dataReading = array();
$dataSetPoint = array();

$thermostatInfo = GetThermostatInfo($thermostat);

$data = array('thermostat' => $thermostatInfo);

$mysqli = new mysqli($dbUrl, $dbUser, $dbPass, $dbName);

if ($mysqli->connect_error) {
	die('Connect Error (' . $mysqli->connect_errno . ') ' . $mysqli->connect_error);
}		

$thermostatInfo = GetThermostatInfo($thermostat);
$timezone = $thermostatInfo['time_zone'];

$query = "SET time_zone = '$timezone';";	
$mysqli->query($query);

if($privileges->CanReadCurrent())
{
	$query = "SELECT timestamp, celsius, CURRENT_TIMESTAMP as 'now' FROM $tableCurrentTemperature WHERE thermostat = '$thermostat' ORDER BY timestamp DESC LIMIT 1;";

	if($result = $mysqli->query($query))
	{
		while($row = $result->fetch_assoc())
		{
			$currentCelsius = $row['celsius'];	
			$curerntTimestamp = $row['timestamp'];			
			$currentFarenheit = strval(($currentCelsius * 9) / 5 + 32);
			
			$row['farenheit'] = $currentFarenheit;

			$data['current'] = $row;
//			array_push($dataReading,$row);
		}
		
		$result->free();
	}
}
else
{
	$data['current'] = array("error" => "ERR_NO_READ_CURRENT_PRIVILEGES");
}

if($privileges->CanReadTarget())
{
	$query = "SELECT timestamp, celsius, power FROM $tableTargetTemperature WHERE thermostat = '$thermostat' ORDER BY timestamp DESC LIMIT 1;";

	if($result = $mysqli->query($query))
	{
		while($row = $result->fetch_assoc())
		{
			$celsius = $row['celsius'];	
			$timestamp = $row['timestamp'];			
			$farenheit = strval(($celsius * 9) / 5 + 32);
			$row['farenheit'] = $farenheit;

			$data['target'] = $row;
//			array_push($dataSetPoint,$row);
		}		

		$result->free();
	}	
}
else
{
	$data['target'] = array("error" => "ERR_NO_READ_CURRENT_PRIVILEGES");
}

$data['error'] = $errorMessage;
echo json_encode($data);
$mysqli->close(); 
?>		
