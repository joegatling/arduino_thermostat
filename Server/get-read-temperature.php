<?php
include_once 'common.php';

$thermostat = "default";
if(isset($_GET['thermostat']))
{
	$thermostat = $_GET['thermostat'];
}

$mysqli = new mysqli($dbUrl, $dbUser, $dbPass, $dbName);

if ($mysqli->connect_error) {
	die('Connect Error (' . $mysqli->connect_errno . ') ' . $mysqli->connect_error);
}		

$currentTemp = 0;

$thermostatInfo = GetThermostatInfo($thermostat);
$timezone = $thermostatInfo['time_zone'];	

$query = "SET time_zone = '$timezone';";
$mysqli->query($query);

$query = "SELECT timestamp, celsius FROM $tableCurrentTemperature ORDER BY timestamp DESC LIMIT 1;";

$data = array();

if($result = $mysqli->query($query))
{
	while($row = $result->fetch_assoc())
	{
		$celsius = $row['celsius'];	
		$timestamp = $row['timestamp'];			
		$farenheit = strval(($celsius * 9) / 5 + 32);
		$row['farenheit'] = $farenheit;
		array_push($data,$row);
	}
	
	$result->free();
}

echo json_encode(array('data' => $data));

$mysqli->close(); 
?>		
