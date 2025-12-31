<?php
include("conn.php");

echo "<h2 align='center'>数据库中的所有表</h2>";
echo "<table border='1' align='center' cellpadding='8'>";
echo "<tr><th>表名</th></tr>";

$sql = "SHOW TABLES";
$result = $conn->query($sql);

while ($row = $result->fetch_array()) {
    $table = $row[0];
    echo "<tr><td><a href='view_table.php?table=$table'>$table</a></td></tr>";
}

echo "</table>";
$conn->close();
?>
