<?php
include 'database.php';
global $primaryKey;
$tableNames=getTableNames();
echo '<h1>数据库表名</h1>';
foreach ($tableNames as $name){
    echo '<a href="?table=' . $name . '">' . $name . '</a><br>';
}
if (isset($_GET['table'])) {
    $tableName = $_GET['table'];
    setPrimaryKey($tableName);
    if (isset($_POST['add'])) {
        // 添加数据
        $data = $_POST;
        unset($data['add']);
        insertRecord($tableName, $data);
    } elseif (isset($_POST['delete'])) {
        // 删除数据
        $id = $_POST['id'];
        deleteRecord($tableName, $id);
    } elseif (isset($_POST['update'])) {
        // 修改数据
        $id = $_POST['id'];
        $data = $_POST;
        unset($data['update'], $data['id']);
        updateRecord($tableName, $id, $data);
    }

    $tableData = getTableData($tableName);
} else {
    $tableNames = getTableNames();
}
?>

<!DOCTYPE html>
<html>
<head>
    <title>MySQL 数据库操作</title>
</head>
<body>
<?php if (isset($tableData)) { ?>
    <h1><?php echo $tableName; ?></h1>
    <table>
        <tr>
            <?php foreach (array_keys($tableData[0]) as $key) { ?>
                <th><?php echo $key; ?></th>
            <?php } ?>
            <th>操作</th>
        </tr>
        <?php foreach ($tableData as $row) { ?>
            <form method="post">
                <tr>
                    <?php foreach ($row as $key => $value) { ?>
                        <td>
                            <input type="text" name="<?php echo $key; ?>" value="<?php echo $value; ?>">
                        </td>
                    <?php } ?>
                    <td>
                        <input type="hidden" name="id" value="<?php echo $row[$primaryKey]; ?>">
                        <input type="submit" name="update" value="更新">
                        <input type="submit" name="delete" value="删除">
                    </td>
                </tr>
            </form>
        <?php } ?>
        <form method="post">
            <tr>
                <?php foreach (array_keys($tableData[0]) as $key) { ?>
                    <td>
                        <input type="text" name="<?php echo $key; ?>" placeholder="<?php echo $key; ?>">
                    </td>
                <?php } ?>
                <td>
                    <input type="submit" name="add" value="添加">
                </td>
            </tr>
        </form>
    </table>
<?php }?>
</body>
</html>
