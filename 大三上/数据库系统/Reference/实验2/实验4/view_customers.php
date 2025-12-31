<?php
include 'database.php';
$tableData = getBestCustomers();
?>

<!DOCTYPE html>
<html>
<head>
    <title>最佳客户</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: beige;
        }
        header {
            background-color:antiquewhite;
            color:darkgoldenrod;
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
        tr{
            background-color: beige;
        }

        tr:nth-child(even) {
            background-color: #f2f2f2;
        }

        tr:hover {
            background-color: #ddd;
        }

    </style>
</head>
<body>
<header>
    <h1>Best Customers</h1>
</header>
<div class="container">
    <table>
        <tr>
            <th>last_name</th>
            <th>first_name</th>
            <th>address</th>
            <th>telephone</th>
            <th>total_length_of_rental</th>
        </tr>
        <?php foreach ($tableData as $row) { ?>
            <tr>
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