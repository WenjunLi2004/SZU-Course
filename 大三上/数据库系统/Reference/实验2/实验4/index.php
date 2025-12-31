<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            background-image: url('image/background.jpg'); /* 替换为实际的背景图片路径 */
            background-size: cover;
            height: 100vh;
            display: flex;
            flex-wrap: wrap;
            align-items: center;
            justify-content: center;
        }

        .bag {
            width: 177px;
            height: 177px;
            background-color: rgba(255, 255, 255, 0.7);
            margin: 10px;
            padding: 20px;
            text-align: center;
            border-radius: 10px;
            transition: background-color 0.3s ease;
            cursor: pointer;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
        }

        .bag h2 {
            margin: 0;
            font-size: 1.5em;
        }

        .bag:hover {
            background-color: rgba(250, 235, 215, 0.3);
        }
    </style>
    <title>包包租贷系统</title>
</head>
<body>

<div class="bag" onclick="window.location.href='add_customer.php'">
    <h2>添加客户</h2>
</div>

<div class="bag" onclick="window.location.href='LuxuryBags.php'">
    <h2>租贷包包</h2>
</div>

<div class="bag" onclick="window.location.href='return_bag.php'">
    <h2>归还包包</h2>
</div>
<div class="bag" onclick="window.location.href='add_bag.php'">
    <h2>添加包包</h2>
</div>
<div class="bag" onclick="window.location.href='view_customers.php'">
    <h2>最佳客户</h2>
</div>

</body>
</html>
