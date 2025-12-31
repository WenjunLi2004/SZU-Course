<?php
include 'database.php';
global $conn;
if (isset($_POST['return'])) {
    $id = $_POST['rental_id'];
    $sql = "update rental set return_date=current_timestamp where rental_id=$id";
    $result = $conn->query($sql);
    if ($result == 1) {
        $sql = "select datediff(return_date,rental_date) as time from rental where rental_id=$id";
        $result = $conn->query($sql);
        $row = $result->fetch_assoc();
        $time = $row['time'];
        $sql = "select price from designer where designer_id = (select designer_id from bag where bag_id=(select bag_id from rental where rental_id=$id))";
        $result = $conn->query($sql);
        $row = $result->fetch_assoc();
        $price = $row['price'];
        echo '<script>alert("包包归还成功\n租贷总时长为' . $time . '天，消费金额为$' . $time * $price . '")</script>';
    } else
        echo '<script>alert("包包归还失败")</script>';
}
$tableData = getTableData('rental');
$has=getNoReturnLength();
?>

<!DOCTYPE html>
<html>
<head>
    <title>归还包包</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: beige;
        }

        header {
            background-color: antiquewhite;
            color: darkgoldenrod;
            padding: 10px;
            text-align: center;
            border-radius: 20px;
        }

        .container {
            margin-top: 50px;
            background-color: antiquewhite;
            display: flex;
            flex-wrap: wrap;
            align-items: center;
            justify-content: center;
            border-radius: 20px;
            padding-bottom: 10px;
        }

        table {
            width: 80%;
        }

        th, td {
            padding: 5px;
            text-align: center;
            border-radius: 20px;
        }

        th {
            background-color: darkgoldenrod;
            color: #fff;
        }

        tr {
            background-color: beige;
        }

        tr:nth-child(even) {
            background-color: #f2f2f2;
        }

        tr:hover {
            background-color: #ddd;
        }

        input[type="submit"] {
            font-size: 16px;
            cursor: pointer;
            background-color: coral;
            border-radius: 10px;
            border: none;
            width: 70px;
            height: 25px;
        }

    </style>
    <script>
    </script>
</head>
<body>
<header>
    <h1>Return Bags</h1>
</header>
<div class="container">
    <?php if ($has>0) {?>
        <table>
            <tr>
                <?php foreach (getTableHeader('rental') as $key) { ?>
                    <th><?php echo $key; ?></th>
                <?php } ?>
            </tr>
            <?php foreach ($tableData as $row) { ?>
                <form method="post">
                    <?php if ($row['return_date'] == NULL) { ?>
                        <?php foreach ($row as $key => $value) { ?>
                            <?php if ($key == 'return_date') { ?>
                                <td>
                                    <input type="hidden" name="rental_id" value="<?php echo $row['rental_id'] ?>">
                                    <input type="submit" name="return" value="return">
                                </td>
                            <?php } else { ?>
                                <td>
                                    <?php echo $value; ?>
                                </td>
                            <?php } ?>
                        <?php } ?>
                        </tr>
                    <?php } ?>
                </form>
            <?php } ?>
        </table>
    <?php } else { ?>
        <header>
            <h2>There is no rental.</h2>
        </header>
    <?php }?>
</div>
</body>
</html>