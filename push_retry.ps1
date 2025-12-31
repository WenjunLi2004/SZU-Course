$maxRetries = 1000
$count = 0
$ErrorActionPreference = "Continue"

# Configure git for large files
git config --global http.postBuffer 524288000
git config --global http.lowSpeedLimit 1000
git config --global http.lowSpeedTime 600

while ($count -lt $maxRetries) {
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    Write-Host "[$timestamp] Attempting git push (Attempt $($count + 1))..." -ForegroundColor Cyan
    
    # Run git push
    git push
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[$timestamp] Git push successful!" -ForegroundColor Green
        break
    }
    
    Write-Host "[$timestamp] Git push failed with exit code $LASTEXITCODE. Retrying in 5 seconds..." -ForegroundColor Yellow
    Start-Sleep -Seconds 5
    $count++
}
