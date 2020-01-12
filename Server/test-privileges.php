<?php
include_once 'common.php';

//$apiKey = '2b187604-9d32-4047-8028-6185fbc7a1be';
$apiKey = '6df657ff-eac2-4155-99f5-42ca02d6c207';


$privileges = GetPrivileges($apiKey);

echo("key = $apiKey </br>");

if($privileges->CanWriteCurrent())
{
    echo ("Can write current temp: TRUE<br/>");
}
else
{
    echo ("Can write current temp: FALSE<br/>");
}

if($privileges->CanReadCurrent())
{
    echo ("Can read current temp: TRUE<br/>");
}
else
{
    echo ("Can read current temp: FALSE<br/>");
}

if($privileges->CanWriteTarget())
{
    echo ("Can write target temp: TRUE<br/>");
}
else
{
    echo ("Can write target temp: FALSE<br/>");
}

if($privileges->CanReadTarget())
{
    echo ("Can read target temp: TRUE<br/>");
}
else
{
    echo ("Can read target temp: FALSE<br/>");
}
?>