<?php
include_once 'config.php';

$rowsToKeep = 100000;

$thermostat = "default";
if(isset($_GET['thermostat']))
{
	$thermostat = $_GET['thermostat'];
}

$mysqli = new mysqli($dbUrl, $dbUser, $dbPass, $dbName);

if ($mysqli->connect_error) {
	die('Connect Error (' . $mysqli->connect_errno . ') ' . $mysqli->connect_error);
}		

$query = "SELECT @ROWS_TO_DELETE := COUNT(*) - $rowsToKeep FROM `$tableCurrentTemperature`; SELECT @ROWS_TO_DELETE := IF(@ROWS_TO_DELETE<0,0,@ROWS_TO_DELETE); PREPARE STMT FROM \"DELETE FROM `$tableCurrentTemperature` ORDER BY `timestamp` ASC LIMIT ?\"; EXECUTE STMT USING @ROWS_TO_DELETE;";

$mysqli->query($query);
$mysqli->close(); 
?>		
