<?php
$dbUser = "nodemcu";
$dbPass = "netxaX-zabtej-6qucmy";
$dbUrl = "joegatling.powwebmysql.com";
$dbName = "joegatling_thermostat";
$table = "temperature_read";

$zoneId = "default";
$sinceTimestamp = 0;
$thermostat = "beachwood";

if(isset($_GET['thermostat']))
{
	$thermostat = $_GET['thermostat'];
}


$timezoneoffset=0;


$mysqli = new mysqli($dbUrl, $dbUser, $dbPass, $dbName);

if ($mysqli->connect_error) {
	die('Connect Error (' . $mysqli->connect_errno . ') ' . $mysqli->connect_error);
}		

$query = "SELECT date(timestamp) as date, hour(timestamp) as hour, (floor(minute(timestamp)/2)*2) as minute, avg(celsius) as celsius\n"
    . "from temperature_read\n"
    . "where timestamp > (now() - interval 24 hour)\n"
    . "group by date, hour, minute;";

$rows = array();


if($result = $mysqli->query($query))
{
	while($row = $result->fetch_assoc())
	{		
		$newRow = array();
		//$dateString = $row['date'] . " " . $row['hour'] . ":" .  $row['minute'];
		$newRow['c'] = array();

		array_push($newRow['c'], array('v' => $row['date']  . " " . str_pad($row['hour'], 2, "0", STR_PAD_LEFT) . ":" .  str_pad($row['minute'], 2, "0", STR_PAD_LEFT)));
		array_push($newRow['c'], array('v' => $row['celsius']));

		array_push($rows,$newRow);
		// $celsius = $row['celsius'];	
		// $timestamp = $row['time'];	
		
		// $farenheit = ($celsius * 9) / 5 + 32;
	}
	
	$result->free();
}
else
{
	echo("<p>No Result</p>");
}

$cols = array();

$dateCol = array();
$dateCol['type'] = 'string';
$dateCol['label'] = "Timestamp";

$tempCol = array();
$tempCol['type'] = 'number';
$tempCol['label'] = "Celsius";

array_push($cols, $dateCol);
array_push($cols, $tempCol);



echo json_encode(array('cols' => $cols, 'rows' => $rows));

$mysqli->close(); 
?>		
