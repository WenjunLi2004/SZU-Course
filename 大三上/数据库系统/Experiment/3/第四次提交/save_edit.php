<?php
include("conn.php");

$table = $_POST['table'];
$pk = $_POST['pk'];
$id = $_POST['id'];

$updates = [];
foreach ($_POST as $col => $val) {
    if (!in_array($col, ['table','pk','id'])) {
        $updates[] = "$col='" . $conn->real_escape_string($val) . "'";
    }
}

$sql = "UPDATE $table SET " . implode(",", $updates) . " WHERE $pk='$id'";
if ($conn->query($sql)) {
    echo "<script>alert('修改成功！'); window.location='view_table.php?table=$table';</script>";
} else {
    echo "修改失败: " . $conn->error;
}
?>
