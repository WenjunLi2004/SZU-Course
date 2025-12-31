<?php
$servername = "localhost";
$username = "root";
$password = "";
$dbname = "store_db";

$conn = new mysqli($servername, $username, $password, $dbname);
if ($conn->connect_error) {
    die("数据库连接失败: " . $conn->connect_error);
}
$conn->set_charset("utf8mb4");
?>