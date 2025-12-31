<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>租贷包包</title>
    <style>
        header {
            background-color: antiquewhite;
            color: darkgoldenrod;
            padding: 10px;
            text-align: center;
            border-radius: 10px;
        }

        form {
            margin: 10px 0;
            text-align: center;
            background-color: antiquewhite;
            padding: 10px;
            border-radius: 10px;
            display: flex;
            flex-direction: row;
            column-gap: 20px;
            justify-content: center;
        }

        label {
            font-weight: bold;
            color: darkgoldenrod;
        }

        select {
            font-size: 16px;
            border: none;
            border-radius: 5px;
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

        .container {
            background-color: antiquewhite;
            display: flex;
            flex-wrap: wrap;
            align-items: center;
            justify-content: center;
            border-radius: 10px;
        }

        .bag {
            width: 250px;
            height: 250px;
            background-color: rgba(255, 255, 255, 0.7);
            margin: 20px;
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

        .bag img {
            width: 90%;
            height: 90%;
        }

        .bag:hover {
            background-color: rgba(0, 0, 0, 0.2);
        }

    </style>
</head>
<body>
<header>
    <h1>Leasing luxury</h1>
</header>
<form method="post">
    <label for="colorFilter">Color:</label>
    <select name="colorFilter" id="colorFilter">
        <option value="all">All Colors</option>
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
    <label for="designerFilter">Designer:</label>
    <select name="designerFilter" id="designerFilter">
        <option value="all">All Designers</option>
        <option value="Louis Vuitton">Louis Vuitton</option>
        <option value="Coach">Coach</option>
        <option value="Prada">Prada</option>
        <option value="Burberry">Burberry</option>
    </select>
    <input type="submit" value="Select">
</form>
<div class="container">
    <?php
    include 'database.php';
    global $conn;
    if (isset($_GET['customer_id'])) {
        $optional_insurance = $_GET['optional_insurance'];
        $customer_id = $_GET['customer_id'];
        $bag_id=$_GET['bag_id'];
        $ID = getTableLength('rental')+1;
        $sql = "call add_rental($ID, '$customer_id', $bag_id, $optional_insurance)";
        $result = $conn->query($sql);
        if ($result == 1)
            echo '<script>alert("包包租借成功")</script>';
        else
            echo '<script>alert("包包租借失败")</script>';
    }
    $sql = "SELECT * FROM bag";
    if (isset($_POST['colorFilter'])) {
        $colorFilter = $_POST['colorFilter'];
        if ($colorFilter !== 'all')
            $sql = "SELECT * FROM bag WHERE color = '$colorFilter'";
    }
    if (isset($_POST['designerFilter'])) {
        $designerFilter = $_POST['designerFilter'];
        $id = getDesignerID($designerFilter);
        if ($designerFilter !== 'all')
            $sql = "SELECT * FROM bag WHERE designer_id = $id";
    }
    $result = $conn->query($sql);
    if ($result->num_rows > 0) {
        while ($row = $result->fetch_assoc()) {
            if ($row['rent'] == 0) {
                $url = "rend.php?bag_id=" . urlencode($row["bag_id"]);
                echo '<div class="bag" onclick="window.location.href=\'' . $url . '\'">';
                echo '<text style="padding-top: 10px">' . $row["type"] . '</text>';
                echo '<img src="' . $row["image"] . '" alt="' . $row["type"] . '">';
                echo '<text style="padding-bottom: 10px">$' . getPrice($row['designer_id']) . '</text>';
                echo '</div>';
            }
        }
    } else {
        echo "0 results";
    }
    $conn->close();
    ?>
</div>
</body>
</html>