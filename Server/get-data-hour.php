<?php
include_once 'common.php';

$zoneId = "default";
$sinceTimestamp = 0;
$thermostat = "beachwood";

if(isset($_GET['thermostat']))
{
	$thermostat = mysqli_real_escape_string($_GET['thermostat']);
}


$timezoneoffset=0;


$mysqli = new mysqli($dbUrl, $dbUser, $dbPass, $dbName);

if ($mysqli->connect_error) {
	die('Connect Error (' . $mysqli->connect_errno . ') ' . $mysqli->connect_error);
}		

$thermostatInfo = GetThermostatInfo($thermostat);
$timezone = $thermostatInfo['time_zone'];

$query = "SET time_zone = '$timezone';";
$mysqli->query($query);

$query = "SELECT max(timestamp) INTO @maxTimestamp from $tableCurrentTemperature where thermostat = '$thermostat' limit 1;"; 
$mysqli->query($query);

$query = "SELECT celsius, TIMESTAMPDIFF(SECOND, @maxTimestamp, timestamp) as offset FROM $tableCurrentTemperature WHERE thermostat = '$thermostat' AND timestamp > (now() - interval 1 hour)";
//$query = "SELECT celsius, TIMESTAMPDIFF(MINUTE, @maxTimestamp, timestamp) as offset FROM $tableCurrentTemperature WHERE thermostat = '$thermostat' AND timestamp > (now() - interval 1 hour)";

// 	SELECT timestamp as t, date(timestamp) as date, hour(timestamp) as hour, (floor(minute(timestamp)/1)*1) as minute, avg(celsius) as celsius, (SELECT celsius from temperature_set where timestamp < t order by timestamp desc limit 1) as target, TIMESTAMPDIFF(MINUTE, now(), now()) as DIFF	
// from temperature_read
// where timestamp > (now() - interval 24 hour) and thermostat = 'beachwood'
// group by date, hour, minute	

// echo $query; 
$data = array();


if($result = $mysqli->query($query))
{
	while($row = $result->fetch_assoc())
	{		
		$newRow = array();
		
		
//		var_dump($row);

		$newRow['offset'] = $row['offset'];
		$newRow['celsius'] =  $row['celsius'];

		array_push($data,$newRow);
	}
	
	$result->free();
}


echo json_encode($data);

$mysqli->close(); 
?>		
