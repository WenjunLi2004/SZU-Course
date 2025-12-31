<?php
include("conn.php");

$table = $_GET['table'];
echo "<h2 align='center'>表：$table</h2>";

$sql = "SELECT * FROM $table";
$result = $conn->query($sql);

if ($result->num_rows > 0) {
    echo "<table border='1' align='center' cellpadding='5'>";
    
    // 显示表头
    $fields = $result->fetch_fields();
    echo "<tr>";
    foreach ($fields as $field) {
        echo "<th>{$field->name}</th>";
    }
    echo "<th>操作</th></tr>";
    
    // 显示记录
    while ($row = $result->fetch_assoc()) {
        echo "<tr>";
        foreach ($row as $v) {
            echo "<td>$v</td>";
        }
        // 获取主键列
        $pk = $fields[0]->name;
        $id = $row[$pk];
        echo "<td>
            <a href='edit_record.php?table=$table&id=$id'>修改</a> |
            <a href='delete_record.php?table=$table&id=$id' onclick='return confirm(\"确定删除?\")'>删除</a>
        </td>";
        echo "</tr>";
    }
    echo "</table>";
} else {
    echo "<p align='center'>暂无数据</p>";
}

echo "<p align='center'><a href='add_record.php?table=$table'>➕ 添加新记录</a></p>";
echo "<p align='center'><a href='index.php'>⬅ 返回表列表</a></p>";

$conn->close();
?>
