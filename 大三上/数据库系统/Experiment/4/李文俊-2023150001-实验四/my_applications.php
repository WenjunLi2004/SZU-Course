<?php
include("conn.php");
include("header.php");
if($_SESSION['role'] != 'USER') die("权限不足");

$uid = $_SESSION['user_id'];
// 需求5：用户查看状态 (直接查视图)
$sql = "SELECT * FROM v_user_application_details WHERE user_id = $uid";
$result = $conn->query($sql);
?>

<div class="card">
    <div class="card-header bg-white">
        <h4>📄 我的报名记录</h4>
    </div>
    <div class="card-body">
        <table class="table table-hover table-striped">
            <thead>
                <tr>
                    <th>申请ID</th>
                    <th>活动名称</th>
                    <th>活动地点</th>
                    <th>申请时间</th>
                    <th>当前状态</th>
                    <th>操作</th>
                </tr>
            </thead>
            <tbody>
                <?php while($row = $result->fetch_assoc()): ?>
                <tr>
                    <td>#<?php echo $row['application_id']; ?></td>
                    <td><b><?php echo $row['activity_title']; ?></b></td>
                    <td><?php echo $row['location']; ?></td>
                    <td><?php echo $row['apply_time']; ?></td>
                    <td>
                        <?php 
                        $status = $row['application_status']; // 假设是中文 "待审核"/"通过"
                        $badge = 'bg-secondary';
                        if(strpos($status, '通过') !== false || strpos($status, 'APPROVED') !== false) $badge = 'bg-success';
                        if(strpos($status, '拒绝') !== false || strpos($status, 'REJECTED') !== false) $badge = 'bg-danger';
                        if(strpos($status, '待') !== false || strpos($status, 'PENDING') !== false) $badge = 'bg-warning text-dark';
                        ?>
                        <span class="badge <?php echo $badge; ?>"><?php echo $status; ?></span>
                    </td>
                    <td>
                        <?php 
                        // 只有 PENDING(待审核) 或 APPROVED(已通过) 且活动未结束时可以撤销 
                        // 这里简单判断状态，只有包含 PENDING 或 APPROVED 的才能撤销 
                        $can_withdraw = (strpos($status, 'PENDING') !== false || strpos($status, 'APPROVED') !== false);
                        
                        if($can_withdraw): 
                        ?>
                            <a href="action.php?act=withdraw&aid=<?php echo $row['activity_id']; ?>" 
                               class="btn btn-sm btn-outline-danger" 
                               onclick="return confirm('确定要撤销报名吗？如果名额紧张，撤销后可能无法再次报名。');">
                               ↩️ 撤销
                            </a>
                        <?php else: ?>
                            <span class="text-muted small">无法撤销</span>
                        <?php endif; ?>
                    </td>
                </tr>
                <?php endwhile; ?>
            </tbody>
        </table>
    </div>
</div>
</body>
</html>