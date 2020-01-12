<?php
include_once 'config.php';

class Privileges
{
    private $canWriteCurrent = false;
    private $canReadCurrent = false;

    private $canWriteTarget = false;
    private $canReadTarget = false;

    function __construct($writeCurrent, $readCurrent, $writeTarget, $readTarget)
    {
        $this->canWriteCurrent = boolval($writeCurrent);
        $this->canReadCurrent = boolval($readCurrent);

        $this->canWriteTarget = boolval($writeTarget);
        $this->canReadTarget = boolval($readTarget);
    }

    function CanWriteCurrent()
    {
        return $this->canWriteCurrent;
    }

    function CanReadCurrent()
    {
        return $this->canReadCurrent;
    }

    function CanWriteTarget()
    {
        return $this->canWriteTarget;
    }

    function CanReadTarget()
    {
        return $this->canReadTarget;
    }
}

function GetPrivileges($apiKey)
{
    $writeCurrent = false;
    $readCurrent = false;
    $writeTarget = false;
    $readTarget = false;

    global $dbUser;
    global $dbPass;
    global $dbUrl;
    global $dbName;
    
    global $tableApiKeys;

	$mysqli = new mysqli($dbUrl, $dbUser, $dbPass, $dbName);
	
    if ($mysqli->connect_error) 
    {
		die('Connect Error (' . $mysqli->connect_errno . ') ' . $mysqli->connect_error);
	}		

	$query = "SELECT * FROM $tableApiKeys WHERE api_key = '$apiKey' LIMIT 1;"; 

	if($result = $mysqli->query($query))
	{
        while($row = $result->fetch_assoc())
		{
            $writeCurrent = boolval($row['write_current_temp']);
            $readCurrent = boolval($row['read_current_temp']);
            $writeTarget = boolval($row['write_target_temp']);
            $readTarget = boolval($row['read_target_temp']);
        }
        
        $result->free();
	}

    $mysqli->close();	    
    
    $privileges = new Privileges($writeCurrent, $readCurrent, $writeTarget, $readTarget);
    return $privileges;
}


?>