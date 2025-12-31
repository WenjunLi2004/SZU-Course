<?php
include 'database.php';
global $primaryKey;
global $database;
$tableNames = getTableNames();
if (isset($_GET['table'])) {
    $tableName = $_GET['table'];
    setPrimaryKey($tableName);
    if (isset($_POST['add'])) {
        // 添加数据
        $data = $_POST;
        unset($data['add']);
        echo '<script>alert("'.insertRecord($tableName, $data).'")</script>';
    } elseif (isset($_POST['delete'])) {
        // 删除数据
        $key = $_POST['key'];
        echo '<script>alert("'.deleteRecord($tableName, $key).'")</script>';
    } elseif (isset($_POST['update'])) {
        // 修改数据
        $key = $_POST['key'];
        $data = $_POST;
        unset($data['update'], $data['key']);
        echo '<script>alert("'.updateRecord($tableName, $key, $data).'")</script>';
    }
    $tableData = getTableData($tableName);
}
?>

<!DOCTYPE html>
<html>
<head>
    <title>数据库管理系统</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
            background-color: #f5f5f5;
        }

        h2 {
            background-color: #333;
            color: #fff;
            text-align: center;
            padding: 10px;
        }

        table {
            border-collapse: collapse;
            width: 80%;
            margin: auto;
            background-color: #fff;
        }

        th, td {
            padding: 10px;
            text-align: center;
        }

        th {
            background-color: #333;
            color: #fff;
        }

        tr:nth-child(even) {
            background-color: #f2f2f2;
        }

        tr:hover {
            background-color: #ddd;
        }

        form {
            display: inline;
        }

        input {
            text-align: center;
        }

        input[type="text"] {
            width: 100%;
            padding: 5px;
        }

        input[type="submit"] {
            background-color: #333;
            color: #fff;
            border: none;
            padding: 5px 10px;
            cursor: pointer;
        }

        input[type="submit"]:hover {
            background-color: #555;
        }
    </style>
</head>
<body>
<h2>Database <?php echo $database ?></h2>
<table>
    <?php foreach ($tableNames as $name) { ?>
        <tr>
            <td><?php echo '<a href="?table=' . $name . '">' . $name . '</a>'; ?></td>
        </tr>
    <?php } ?>
</table>
<?php if (isset($tableData)) { ?>
    <h2 colspan=100%><?php echo 'Table ' . $tableName; ?></h2>
    <table>
        <tr>
            <?php foreach (getTableHeader($tableName)as $key) { ?>
                <th><?php echo $key; ?></th>
            <?php } ?>
            <th>操作</th>
        </tr>
        <?php if (!empty($tableData)) { ?>
            <?php foreach ($tableData as $row) { ?>
                <form method="post">
                    <tr>
                        <?php foreach ($row as $key => $value) { ?>
                            <td>
                                <input type="text" name="<?php echo $key; ?>" value="<?php echo $value; ?>">
                            </td>
                        <?php } ?>
                        <td>
                            <input type="hidden" name="key" value="<?php echo $row[$primaryKey]; ?>">
                            <input type="submit" name="update" value="更新">
                            <input type="submit" name="delete" value="删除">
                        </td>
                    </tr>
                </form>
            <?php } ?>
        <?php } ?>
        <form method="post">
            <tr>
                <?php foreach (getTableHeader($tableName) as $key) { ?>
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
<?php } ?>
</body>
</html>
