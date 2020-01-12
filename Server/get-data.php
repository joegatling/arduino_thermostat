<?php
$dbUser = "nodemcu";
$dbPass = "netxaX-zabtej-6qucmy";
$dbUrl = "joegatling.powwebmysql.com";
$dbName = "joegatling_thermostat";
$table = "temperature_set";

$zoneId = "default";
$sinceTimestamp = 0;

if(isset($_GET['since']))
{
	$sinceTimestamp = $_GET['since'];
}

$timezoneoffset=0;

$mysqli = new mysqli($dbUrl, $dbUser, $dbPass, $dbName);

if ($mysqli->connect_error) {
	die('Connect Error (' . $mysqli->connect_errno . ') ' . $mysqli->connect_error);
}		

$currentTemp = 0;
$query = "SELECT time, celsius FROM $table WHERE time > $sinceTimestamp ORDER BY time;";

$data = array();

if($result = $mysqli->query($query))
{
	while($row = $result->fetch_assoc())
	{
		array_push($data,$row);
		$celsius = $row['celsius'];	
		$timestamp = $row['time'];	
		
		$farenheit = ($celsius * 9) / 5 + 32;
	}
	
	$result->free();
}
else
{
	echo("<p>No Result</p>");
}


echo json_encode(array('data' => $data));

$mysqli->close(); 
?>		
