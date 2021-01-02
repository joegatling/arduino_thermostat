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

$query = "SELECT timestamp as t, date(timestamp) as date, hour(timestamp) as hour, (floor(minute(timestamp)/5)*5) as minute, avg(celsius) as celsius,\n"
	. "(SELECT celsius from temperature_set where timestamp < t order by timestamp desc limit 1) as target\n"
    . "from temperature_read\n"
    . "where timestamp > (now() - interval 24 hour) and thermostat = '$thermostat'\n"
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
		$fahrenheit = strval(($row['celsius'] * 9) / 5 + 32);
		array_push($newRow['c'], array('v' => $row['celsius'], 'f' => number_format($fahrenheit, 0) . "F / " .number_format($row['celsius'], 1) . "C"));
		$fahrenheit = strval(($row['target'] * 9) / 5 + 32);
		array_push($newRow['c'], array('v' => $row['target'], 'f' => number_format($fahrenheit, 0) . "F / " .number_format($row['target'], 1) . "C"));

		array_push($rows,$newRow);
	}
	
	$result->free();
}

	
$cols = array();

$dateCol = array();
$dateCol['type'] = 'string';
$dateCol['label'] = "Timestamp";

$tempCol = array();
$tempCol['type'] = 'number';
$tempCol['label'] = "Temperature";

$targetCol = array();
$targetCol['type'] = 'number';
$targetCol['label'] = "Set";

array_push($cols, $dateCol);
array_push($cols, $tempCol);
array_push($cols, $targetCol);



echo json_encode(array('cols' => $cols, 'rows' => $rows));

$mysqli->close(); 
?>		
