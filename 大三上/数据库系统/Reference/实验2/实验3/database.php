<?php
$host = "localhost"; // 数据库主机
$username = "root"; // 数据库用户名
$password = ""; // 数据库密码
$database = "taobao"; // 数据库名称
$conn = new mysqli($host, $username, $password, $database);
$primaryKey = "";
if ($conn->connect_error) {  //数据库连接失败弹出提示
    echo '<script>alert("数据库连接失败")</script>';
    die(); //终止代码
}

function setPrimaryKey($tableName)
{
    global $conn;
    global $primaryKey;
    $sql = "show index from $tableName";
    $result = mysqli_query($conn, $sql);
    $row = mysqli_fetch_assoc($result);
    $primaryKey = $row['Column_name'];
}

function getTableNames()
{
    global $conn;
    $sql = "show tables";
    $result = $conn->query($sql);
    $tableNames = array();
    if ($result->num_rows > 0) {
        while ($row = $result->fetch_assoc()) {
            $tableNames[] = $row['Tables_in_' . $GLOBALS['database']];
        }
    }
    return $tableNames;
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

function insertRecord($tableName, $data)
{
    global $conn;
    $keys = implode(', ', array_keys($data)); // 连成字符串
    $values = "'" . implode("', '", array_values($data)) . "'";
    $sql = "insert into $tableName ($keys) values ($values)";
    $result = $conn->query($sql);
    if ($result == 1)
        return '添加成功';
    else
        return '添加失败';
}

function deleteRecord($tableName, $key)
{
    global $conn;
    global $primaryKey;
    $sql = "delete from $tableName where $primaryKey = '$key'";
    $result = $conn->query($sql);
    if ($result == 1)
        return '删除成功';
    else
        return '删除失败';
}

function updateRecord($tableName, $pkey, $data)
{
    global $conn;
    global $primaryKey;
    $set = "";
    foreach ($data as $key => $value) {
        $set .= "$key = '$value', ";
    }
    $set = rtrim($set, ', '); // 移除最后一个字符','
    $sql = "update $tableName set $set where $primaryKey = '$pkey'";
    $result = $conn->query($sql);
    if ($result == 1)
        return '修改成功';
    else
        return '修改失败';
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
