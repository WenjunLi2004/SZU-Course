<?php
// 数据库连接信息
$servername = "localhost";
$username = "root";
$password = "";
$dbname = "rbms";
// 创建数据库连接
$conn = new mysqli($servername, $username, $password, $dbname);
// 检查连接是否成功
if ($conn->connect_error) {  //数据库连接失败弹出提示
    echo '<script>alert("数据库连接失败")</script>';
    die(); //终止代码
}
function getPrice($id)
{
    global $conn;
    $sql = "select price from designer where designer_id = $id";
    $result = $conn->query($sql);
    $row = $result->fetch_assoc();
    return $row['price'];
}

function getDesignerID($name)
{
    global $conn;
    $sql = "select designer_id from designer where name = '$name'";
    $result = $conn->query($sql);
    $row = $result->fetch_assoc();
    return $row['designer_id'];
}

function getBestCustomers()
{
    global $conn;
    $sql = "call BestCustomers()";
    $result = $conn->query($sql);
    $tableData = array();
    if ($result->num_rows > 0) {
        while ($row = $result->fetch_assoc()) {
            $tableData[] = $row;
        }
    }
    return $tableData;
}

function getNoReturnLength()
{
    global $conn;
    $sql = "select count(*) from rental where return_date is NULL";
    $result = $conn->query($sql);
    $row = $result->fetch_assoc();
    return $row['count(*)'];
}

function getTableLength($tableName)
{
    global $conn;
    $sql = "select count(*) from $tableName";
    $result = $conn->query($sql);
    $row = $result->fetch_assoc();
    return $row['count(*)'];
}

function getTableData($tableName)
{
    global $conn;
    $sql = "select*from $tableName";
    $result = $conn->query($sql);
    $tableData = array();
    if ($result->num_rows > 0) {
        while ($row = $result->fetch_assoc()) {
            $tableData[] = $row;
        }
    }
    return $tableData;
}

function getTableHeader($tableName)
{
    global $conn;
    $sql = "describe $tableName";
    $result = $conn->query($sql);
    $tableHeader = array();
    if ($result) {
        while ($row = $result->fetch_assoc()) {
            $tableHeader[] = $row['Field'];
        }
    }
    return $tableHeader;
}

?>
