<?php
include_once 'common.php';

$errorMessages = array();

$thermostat = $defaultThermostat;
$user = "defaultUser";

$api_key = "";
if(isset($_GET['key']))
{
	$api_key = $_GET['key'];
}
else
{
	array_push($errorMessages, "ERR_NO_KEY");
}

if(isset($_GET['thermostat']))
{
	$thermostat = $_GET['thermostat'];
}

if(isset($_GET['current']))
{
	$currentCelsius = $_GET['current'];
}
else if(isset($_GET['current_c']))
{
	$currentCelsius = $_GET['current_c'];
}
else if(isset($_GET['current_f']))
{
	$currentCelsius = ($_GET['current_f'] - 32) * 5/9;
}

if(isset($_GET['target']))
{
	$targetCelsius = $_GET['target'];
}
else if(isset($_GET['target_c']))
{
	$targetCelsius = $_GET['target_c'];
}
else if(isset($_GET['target_f']))
{
	$targetCelsius = ($_GET['target_f'] - 32) * 5/9;
}

if(isset($_GET['power']))
{
	$isThermostatOn = $_GET['power'];
}

$privileges = GetPrivileges($api_key);
$dataReading = array();
$dataSetPoint = array();

$mysqli = new mysqli($dbUrl, $dbUser, $dbPass, $dbName);

if ($mysqli->connect_error) 
{
	die('Connect Error (' . $mysqli->connect_errno . ') ' . $mysqli->connect_error);
}		

$thermostatInfo = GetThermostatInfo($thermostat);
$data = array('thermostat' => $thermostatInfo);

$timezone = $thermostatInfo['time_zone'];
$query = "SET time_zone = '$timezone';";	
$mysqli->query($query);

if(isset($currentCelsius))
{
	if($privileges->CanWriteCurrent())
	{
		$query = "INSERT INTO $tableCurrentTemperature (thermostat, celsius) VALUES('$thermostat', $currentCelsius)";
		$mysqli->query($query);
	}
	else
	{
		array_push($errorMessages, "ERR_NO_WRITE_CURRENT_PRIVILEGES");
	}
}

if(isset($targetCelsius))
{
	if($privileges->CanWriteTarget())
	{
		$powerSubquery = "";

		if(isset($isThermostatOn))
		{
			$powerSubquery = $isThermostatOn;
		}
		else
		{
			$powerSubquery = "(SELECT * FROM (SELECT power FROM $tableTargetTemperature WHERE thermostat = '$thermostat' ORDER BY timestamp DESC LIMIT 1) as power)";
		}

		$query = "INSERT INTO $tableTargetTemperature (thermostat, user, celsius, power) VALUES('$thermostat', '$user', $targetCelsius, $powerSubquery)";
		$mysqli->query($query);
	}
	else
	{
		array_push($errorMessages, "ERR_NO_WRITE_TARGET_PRIVILEGES");
	}
}


if($privileges->CanReadCurrent())
{
	$query = "SELECT timestamp, celsius FROM $tableCurrentTemperature WHERE thermostat = '$thermostat' ORDER BY timestamp DESC LIMIT 1;";

	if($result = $mysqli->query($query))
	{
		while($row = $result->fetch_assoc())
		{
			$currentCelsius = $row['celsius'];	
			$curerntTimestamp = $row['timestamp'];			
			$currentFarenheit = strval(($currentCelsius * 9) / 5 + 32);
			
			$row['farenheit'] = $currentFarenheit;

			$data['current'] = $row;
		}
		
		$result->free();
	}
}
else
{
	array_push($errorMessages, "ERR_NO_READ_CURRENT_PRIVILEGES");	
	//$data['current'] = array("error" => "ERR_NO_READ_CURRENT_PRIVILEGES");
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
	array_push($errorMessages, "ERR_NO_READ_TARGET_PRIVILEGES");	

//	$data['target'] = array("error" => "ERR_NO_READ_TARGET_PRIVILEGES");
}

$data['error'] = $errorMessages;
//array_push($data['error'], $errorMessages);
echo json_encode($data);
$mysqli->close(); 
?>		
