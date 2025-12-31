<?php
include("conn.php");
$table = $_GET['table'];

$columns = [];
$res = $conn->query("SHOW COLUMNS FROM $table");
while ($r = $res->fetch_assoc()) {
    $columns[] = $r['Field'];
}

echo "<h2 align='center'>添加新记录 ($table)</h2>";
echo "<form method='post' action='add_record.php?table=$table'>";
echo "<table align='center' cellpadding='5'>";
foreach ($columns as $c) {
    echo "<tr><td>$c</td><td><input type='text' name='$c'></td></tr>";
}
echo "<tr><td colspan='2' align='center'><input type='submit' name='submit' value='插入'></td></tr>";
echo "</table></form>";

if (isset($_POST['submit'])) {
    $values = [];
    foreach ($columns as $c) {
        $values[] = "'" . $conn->real_escape_string($_POST[$c]) . "'";
    }
    $sql = "INSERT INTO $table VALUES(" . implode(",", $values) . ")";
    if ($conn->query($sql)) {
        echo "<script>alert('插入成功！'); window.location='view_table.php?table=$table';</script>";
    } else {
        echo "插入失败: " . $conn->error;
    }
}

echo "<p align='center'><a href='view_table.php?table=$table'>⬅ 返回</a></p>";
?>
