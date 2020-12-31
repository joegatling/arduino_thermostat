<?php
$dbUser = "nodemcu";
$dbPass = "netxaX-zabtej-6qucmy";
$dbUrl = "joegatling.powwebmysql.com";
$dbName = "joegatling_thermostat";
$table = "temperature_read";

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
$query = "SELECT timestamp, celsius FROM $table ORDER BY timestamp DESC LIMIT 1;";


$data = array();

if($result = $mysqli->query($query))
{
	while($row = $result->fetch_assoc())
	{
		$celsius = $row['celsius'];	
		$timestamp = $row['time'];			
		$farenheit = strval(($celsius * 9) / 5 + 32);
		$row['farenheit'] = $farenheit;
		array_push($data,$row);
	}
	
	$result->free();
}
else

echo json_encode(array('data' => $data));

$mysqli->close(); 
?>		
