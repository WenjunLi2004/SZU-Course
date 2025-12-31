<?php
include("conn.php");

$table = $_GET['table'];
$id = $_GET['id'];

$pk_query = $conn->query("SHOW KEYS FROM $table WHERE Key_name = 'PRIMARY'");
$pk_row = $pk_query->fetch_assoc();
$pk = $pk_row['Column_name'];

$sql = "DELETE FROM $table WHERE $pk='$id'";
if ($conn->query($sql)) {
    echo "<script>alert('删除成功！'); window.location='view_table.php?table=$table';</script>";
} else {
    echo "删除失败: " . $conn->error;
}
?>
