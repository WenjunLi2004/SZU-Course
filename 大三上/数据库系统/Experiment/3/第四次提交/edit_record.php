<?php
include("conn.php");
$table = $_GET['table'];
$id = $_GET['id'];

// 获取字段信息
$fields = $conn->query("SHOW COLUMNS FROM $table");
$columns = [];
while ($f = $fields->fetch_assoc()) {
    $columns[] = $f['Field'];
}

// 查询当前记录
$pk = $columns[0]; // 默认第一列为主键
$sql = "SELECT * FROM $table WHERE $pk='$id'";
$row = $conn->query($sql)->fetch_assoc();

echo "<h2 align='center'>修改 $table 表记录</h2>";
echo "<form method='post' action='save_edit.php'>";
echo "<input type='hidden' name='table' value='$table'>";
echo "<input type='hidden' name='pk' value='$pk'>";
echo "<input type='hidden' name='id' value='$id'>";
echo "<table align='center' cellpadding='5'>";

foreach ($row as $col => $val) {
    echo "<tr><td>$col</td><td><input type='text' name='$col' value='$val'></td></tr>";
}

echo "<tr><td colspan='2' align='center'><input type='submit' value='保存修改'></td></tr>";
echo "</table></form>";
echo "<p align='center'><a href='view_table.php?table=$table'>⬅ 返回</a></p>";
?>
