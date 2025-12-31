<?php 
$hostname = "localhost"; //主机名,可以用IP代替
$database = "test"; //数据库名
$username = "root"; //数据库用户名
$password = ""; //数据库密码

// 创建连接
$conn = new mysqli($hostname, $username, $password, $database);

// 检查连接
if ($conn->connect_error) {
    die("连接失败: " . $conn->connect_error);
}

// 设置字符集
$conn->set_charset("utf8");

echo "数据库连接成功";
?>