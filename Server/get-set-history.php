<?php
include_once 'common.php';

$thermostat = "default";
if(isset($_GET['thermostat']))
{
	$thermostat = $_GET['thermostat'];
}

$count = 5;
if(isset($_GET['count']))
{
	$count = $_GET['count'];
}

$mysqli = new mysqli($dbUrl, $dbUser, $dbPass, $dbName);

if ($mysqli->connect_error) {
	die('Connect Error (' . $mysqli->connect_errno . ') ' . $mysqli->connect_error);
}		

// Sanitize inputs
$count = mysqli_real_escape_string($mysqli, $count);
$thermostat = mysqli_real_escape_string($mysqli, $thermostat);

// echo "Count: $count";
// echo "Thermostat: $thermostat";

$currentTemp = 0;

$thermostatInfo = GetThermostatInfo($thermostat);
$timezone = $thermostatInfo['time_zone'];

$query = "SET time_zone = '$timezone';";	
$mysqli->query($query);

$query = "SELECT timestamp, celsius, power, origin FROM $tableTargetTemperature ORDER BY timestamp DESC LIMIT $count;";

$data = array('thermostat' => $thermostatInfo);
$data['history'] = array();

if($result = $mysqli->query($query))
{
	while($row = $result->fetch_assoc())
	{
		//echo($row);
		$celsius = $row['celsius'];	
		//$timestamp = $row['timestamp'];			
		$farenheit = strval(($celsius * 9) / 5 + 32);
		$row['farenheit'] = $farenheit;		
		$data['history'][] = $row;
	}
	
	$result->free();
}
else
{
	echo $mysqli->error;
}

echo json_encode($data);

$mysqli->close(); 
?>		
