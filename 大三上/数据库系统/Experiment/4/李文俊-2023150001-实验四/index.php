<?php
include("conn.php");
include("header.php");

// 1. 获取搜索关键词 
$search = isset($_GET['q']) ? trim($_GET['q']) : ''; 

// 2. 构造查询条件 
$where_clause = ""; 
if($search) { 
    // 简单的模糊查询 
    $safe_search = $conn->real_escape_string($search); 
    $where_clause = " WHERE a.title LIKE '%$safe_search%' OR a.location LIKE '%$safe_search%' "; 
} 

$sql = "SELECT a.*, s.status_name, s.status_desc, 
               IFNULL(stats.total_applicants, 0) as total_applicants, 
               IFNULL(stats.approved_count, 0) as approved_count
        FROM Activity a 
        JOIN ActivityStatus s ON a.activity_status_id = s.activity_status_id 
        LEFT JOIN v_activity_statistics stats ON a.activity_id = stats.activity_id 
        $where_clause
        ORDER BY a.created_at DESC";
$result = $conn->query($sql);
?>

<div class="row mb-4 align-items-center">
    <div class="col-md-6">
        <h2>📢 活动大厅 <small class="text-muted fs-6">浏览所有精彩活动</small></h2>
    </div>
    <div class="col-md-6 text-end">
        <form action="index.php" method="get" class="d-flex justify-content-end">
            <input type="text" name="q" class="form-control me-2" style="max-width: 300px;" placeholder="搜索活动名称或地点..." value="<?php echo htmlspecialchars(isset($_GET['q']) ? $_GET['q'] : ''); ?>">
            <button type="submit" class="btn btn-primary text-nowrap">🔍 搜索</button>
            <?php if(isset($_GET['q']) && $_GET['q']): ?>
                <a href="index.php" class="btn btn-outline-secondary ms-2 text-nowrap">重置</a>
            <?php endif; ?>
        </form>
    </div>
</div>

<?php if(isset($_GET['msg'])): ?>
    <div class="alert alert-success alert-dismissible fade show">
        <?php echo htmlspecialchars($_GET['msg']); ?>
        <button type="button" class="btn-close" data-bs-dismiss="alert"></button>
    </div>
<?php endif; ?>

<?php if(isset($_GET['error'])): ?>
    <div class="alert alert-danger alert-dismissible fade show">
        <?php echo htmlspecialchars($_GET['error']); ?>
        <button type="button" class="btn-close" data-bs-dismiss="alert"></button>
    </div>
<?php endif; ?>

<div class="row">
    <?php while($result && $row = $result->fetch_assoc()): ?>
        <div class="col-md-4 mb-4">
            <div class="card h-100 shadow-sm">
                <div class="card-header d-flex justify-content-between align-items-center 
                    <?php echo ($row['status_name']=='OPEN') ? 'bg-success text-white' : 'bg-secondary text-white'; ?>">
                    <h5 class="card-title mb-0"><?php echo $row['title']; ?></h5>
                    <span class="badge bg-light text-dark"><?php echo $row['status_desc']; ?></span>
                </div>
                <div class="card-body">
                    <p class="card-text text-muted"><?php echo $row['description']; ?></p>
                    <ul class="list-group list-group-flush small mb-3">
                        <li class="list-group-item">📍 地点：<?php echo $row['location']; ?></li>
                        <li class="list-group-item">🕒 时间：<?php echo $row['start_time']; ?></li>
                        <li class="list-group-item">
                            👥 进度： 
                            <span class="fw-bold <?php echo ((isset($row['approved_count']) ? $row['approved_count'] : 0) >= $row['capacity']) ? 'text-danger' : 'text-success'; ?>"> 
                                <?php echo intval(isset($row['approved_count']) ? $row['approved_count'] : 0); ?> 
                            </span> 
                            / <?php echo $row['capacity']; ?> (已通过) 
                            
                            <?php if(isset($_SESSION['role']) && $_SESSION['role'] == 'ADMIN'): ?> 
                                <br> 
                                <small class="text-muted">🔥 总申请热度: <?php echo intval(isset($row['total_applicants']) ? $row['total_applicants'] : 0); ?> 人</small> 
                            <?php endif; ?> 
                        </li>
                    </ul>
                    
                    <?php 
                    $my_role = isset($_SESSION['role']) ? strtoupper(trim($_SESSION['role'])) : '';
                    $act_status = strtoupper(trim($row['status_name']));
                    // 注意：这里 approved_count 是只计算了通过的人数
                    $approved_cnt = isset($row['approved_count']) ? $row['approved_count'] : 0;
                    $remaining = $row['capacity'] - $approved_cnt;
                    $is_full = ($remaining <= 0);
                    ?>

                    <?php if($my_role == 'USER'): ?>
                        <?php if($act_status == 'OPEN'): ?>
                            <?php if(!$is_full): ?>
                                <a href="action.php?act=apply&aid=<?php echo $row['activity_id']; ?>" 
                                   class="btn btn-primary w-100">🙋‍♂️ 立即报名</a>
                            <?php else: ?>
                                <button class="btn btn-warning w-100" disabled>⚠️ 名额已满</button>
                            <?php endif; ?>
                        <?php else: ?>
                            <button class="btn btn-secondary w-100" disabled>已结束</button>
                        <?php endif; ?>
                    <?php endif; ?>

                    <?php if($my_role == 'ADMIN'): ?>
                        <div class="d-grid gap-2">
                            <a href="view_applicants.php?aid=<?php echo $row['activity_id']; ?>" 
                               class="btn btn-outline-info">📋 查看申请名单</a>
                            
                            <a href="edit_activity.php?aid=<?php echo $row['activity_id']; ?>" 
                               class="btn btn-outline-primary">✏️ 修改活动</a>
                            
                            <a href="action.php?act=delete_activity&aid=<?php echo $row['activity_id']; ?>" 
                               class="btn btn-outline-danger" 
                               onclick="return confirm('确定删除？');">🗑️ 删除</a>
                        </div>
                    <?php endif; ?>
                </div>
            </div>
        </div>
    <?php endwhile; ?>
</div>
</body>
</html>