<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>添加包包</title>
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
            width: 400px;
            height: 175px;
        }

        label {
            font-weight: bold;
            text-align: center;
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
            justify-content: center;
        }

        .section form {
            margin-top: 90px;
        }

        select {
            font-size: 16px;
            border: none;
            border-radius: 5px;
        }

    </style>
</head>
<body>
<header>
    <h1>Add Bags</h1>
</header>
<?php
include 'database.php';
global $conn;
if ($_SERVER["REQUEST_METHOD"] == "POST") {
    // 处理表单提交
    $colorFilter = $_POST['colorFilter'];
    $designerFilter = $_POST['designerFilter'];
    $name=$_POST['name'];
    $id = getDesignerID($designerFilter);
    $ID = getTableLength('bag');
    $sql = "call add_bag($ID,'$name','$colorFilter',$id,0)";
    $result = $conn->query($sql);
    if ($result == 1)
        echo '<script>alert("添加包包成功")</script>';
    else
        echo '<script>alert("添加包包失败")</script>';
}
?>
<div class="section">
    <form method="post" action="<?php echo htmlspecialchars($_SERVER["PHP_SELF"]); ?>">
        <div class="section">
            <label style="margin-top: 8px;margin-right: 5px" for="name">Name: </label>
            <input type="text" id="name" name="name" required>
        </div>
        <div style="margin-bottom: 25px">
            <label for="colorFilter">Color:</label>
            <select name="colorFilter" id="colorFilter">
                <option value="White">White</option>
                <option value="Multi">Multi</option>
                <option value="Camel">Camel</option>
                <option value="Green">Green</option>
                <option value="blue">Blue</option>
                <option value="black">Black</option>
                <option value="Mauve">Mauve</option>
                <option value="Gold">Gold</option>
                <option value="Plaid">Plaid</option>
            </select>
        </div>
        <div style="margin-bottom: 25px">
            <label for="designerFilter">Designer:</label>
            <select name="designerFilter" id="designerFilter">
                <option value="Louis Vuitton">Louis Vuitton</option>
                <option value="Coach">Coach</option>
                <option value="Prada">Prada</option>
                <option value="Burberry">Burberry</option>
            </select>
        </div>
        <div class="section">
            <button type="submit">Add Bag</button>
        </div>
    </form>
</div>
</body>
</html>
