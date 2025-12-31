<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>用户注册</title>
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

        form {
            background-color: antiquewhite;
            padding: 20px;
            border-radius: 20px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            width: 600px;
            height: 280px;
        }

        label {
            width: 17%;
            text-align: center;
            margin-top: 5px;
        }

        input {
            width: 100%;
            padding: 8px;
            margin-bottom: 16px;
            box-sizing: border-box;
            border-radius: 15px;
            border-color: beige;
            border-width: 1px;;
        }

        button {
            background-color: coral;
            color: #fff;
            padding: 10px;
            border: none;
            border-radius: 15px;
            width: 100px;
            cursor: pointer;
        }

        button:hover {
            background-color: lightsalmon;
        }

        .section {
            display: flex;
            column-gap: 5px;
            justify-content: center;
        }

        .section form {
            margin-top: 90px;
        }

    </style>
</head>
<body>
<header>
    <h1>Registered Customers</h1>
</header>
<?php
include 'database.php';
global $conn;
if ($_SERVER["REQUEST_METHOD"] == "POST") {
    // 处理表单提交
    $lastname = $_POST["lastname"];
    $firstname = $_POST["firstname"];
    $address = $_POST["address"];
    $phone = $_POST["phone"];
    $bankCard = $_POST["bankCard"];
    $email = $_POST["email"];
    $ID = getTableLength('customer');
    $sql = "insert into customer values ($ID,'$lastname','$firstname','$address',$phone,$phone,'$email',$bankCard)";
    $result = $conn->query($sql);
    if ($result == 1)
        echo '<script>alert("注册用户成功")</script>';
    else
        echo '<script>alert("注册用户失败")</script>';
}
?>
<div class="section">
    <form method="post" action="<?php echo htmlspecialchars($_SERVER["PHP_SELF"]); ?>">
        <div class="section">
            <label for="lastname">姓：</label>
            <input type="text" id="lastname" name="lastname" required>
            <label for="firstname">名：</label>
            <input type="text" id="firstname" name="firstname" required>
        </div>
        <div class="section">
            <label for="address">地址：</label>
            <input type="text" id="address" name="address" required>
        </div>
        <div class="section">
            <label for="phone">电话：</label>
            <input type="tel" id="phone" name="phone" required>
        </div>
        <div class="section">
            <label for="bankCard">银行卡号：</label>
            <input type="text" id="bankCard" name="bankCard" required>
        </div>
        <div class="section">
            <label for="email">邮箱：</label>
            <input type="email" id="email" name="email" required>
        </div>
        <div class="section">
            <button type="submit">注册客户</button>
        </div>
    </form>
</div>


</body>
</html>
