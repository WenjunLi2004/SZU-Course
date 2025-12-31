<?php
include 'database.php';
global $conn;
$tableData = getTableData('customer');
if (isset($_GET['bag_id']))
    $bag_id = $_GET['bag_id'];
?>

<!DOCTYPE html>
<html>
<head>
    <title>选择客户</title>
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

    </style>
    <script>
        function rend(data) {
            var datas=data.split(',')
            var customer_id=datas[0],bag_id=datas[1];
            var insuranceNeed = window.confirm("是否需要保险？");
            var optional_insurance=insuranceNeed ? 1 : 0;
            window.location.href = 'LuxuryBags.php?customer_id='+customer_id+'&optional_insurance='+optional_insurance+'&bag_id='+bag_id;
        }
    </script>
</head>
<body>
<header>
    <h1>Chose Customers</h1>
</header>
<div class="container">
    <table>
        <tr>
            <?php foreach (getTableHeader('customer') as $key) { ?>
                <th><?php echo $key; ?></th>
            <?php } ?>
        </tr>
        <?php foreach ($tableData as $row) { ?>
            <tr onclick="rend('<?php echo $row["customer_id"].','.$bag_id; ?>')">
                <?php foreach ($row as $key => $value) { ?>
                    <td>
                        <?php echo $value; ?>
                    </td>
                <?php } ?>
            </tr>
        <?php } ?>
    </table>
</div>
</body>
</html>