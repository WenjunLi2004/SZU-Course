# This is a PowerShell plotting script. The file keeps the original name
# plot.cpp, but it should be executed with:
#   powershell -ExecutionPolicy Bypass -File .\plot.cpp

Add-Type -AssemblyName System.Drawing

$NLabels = @("1e7", "2e7", "3e7", "4e7", "5e7", "6e7", "7e7", "8e7")
$PList = @(1, 2, 4, 8, 16, 32, 64)

# Average execution time in seconds.
# Rows correspond to PList; columns correspond to NLabels.
$TimeData = @(
    @(1.276664,  2.674110,  4.081396,  5.552642,  6.862878,  8.260562,  9.834450,  11.166920),
    @(0.699329,  1.404138,  2.135074,  2.923046,  3.767926,  4.475646,  5.253118,  6.032052),
    @(0.371856,  0.721954,  1.090740,  1.473802,  1.835090,  2.206822,  2.600586,  3.000734),
    @(0.217415,  0.398738,  0.585223,  0.777204,  0.972511,  1.183920,  1.371530,  1.602428),
    @(0.150292,  0.262839,  0.377495,  0.495577,  0.623304,  0.756752,  0.873538,  1.054396),
    @(0.105853,  0.183301,  0.255675,  0.342053,  0.447330,  0.541086,  0.656767,  0.769488),
    @(0.135707,  0.227478,  0.331706,  0.429920,  0.544227,  0.662963,  0.788276,  0.926420)
)

$Colors = @(
    [System.Drawing.Color]::FromArgb(31, 119, 180),
    [System.Drawing.Color]::FromArgb(255, 127, 14),
    [System.Drawing.Color]::FromArgb(44, 160, 44),
    [System.Drawing.Color]::FromArgb(214, 39, 40),
    [System.Drawing.Color]::FromArgb(148, 103, 189),
    [System.Drawing.Color]::FromArgb(140, 86, 75),
    [System.Drawing.Color]::FromArgb(227, 119, 194),
    [System.Drawing.Color]::FromArgb(127, 127, 127)
)

function Get-SpeedupData {
    $speedup = @()
    for ($i = 0; $i -lt $PList.Count; $i++) {
        $row = @()
        for ($j = 0; $j -lt $NLabels.Count; $j++) {
            $row += [double]$TimeData[0][$j] / [double]$TimeData[$i][$j]
        }
        $speedup += ,$row
    }
    return $speedup
}

function Get-EfficiencyData($SpeedupData) {
    $efficiency = @()
    for ($i = 0; $i -lt $PList.Count; $i++) {
        $row = @()
        for ($j = 0; $j -lt $NLabels.Count; $j++) {
            $row += [double]$SpeedupData[$i][$j] / [double]$PList[$i] * 100.0
        }
        $efficiency += ,$row
    }
    return $efficiency
}

function Draw-Marker($Graphics, $Brush, [float]$X, [float]$Y) {
    $size = 7
    $Graphics.FillEllipse($Brush, $X - $size / 2, $Y - $size / 2, $size, $size)
}

function New-LineChart($FileName, $Title, $YLabel, $Data, [bool]$LogScale, [bool]$DrawIdealLine) {
    $width = 1200
    $height = 760
    $left = 95
    $right = 260
    $top = 70
    $bottom = 95
    $plotWidth = $width - $left - $right
    $plotHeight = $height - $top - $bottom

    $bitmap = New-Object System.Drawing.Bitmap($width, $height)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.Clear([System.Drawing.Color]::White)

    $fontTitle = New-Object System.Drawing.Font("Arial", 17, [System.Drawing.FontStyle]::Bold)
    $fontAxis = New-Object System.Drawing.Font("Arial", 11)
    $fontSmall = New-Object System.Drawing.Font("Arial", 10)
    $black = [System.Drawing.Brushes]::Black
    $gridPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(220, 220, 220), 1)
    $axisPen = New-Object System.Drawing.Pen([System.Drawing.Color]::Black, 1.4)

    $values = @()
    for ($i = 0; $i -lt $PList.Count; $i++) {
        for ($j = 0; $j -lt $NLabels.Count; $j++) {
            $values += [double]$Data[$i][$j]
        }
    }
    if ($LogScale) {
        $minVal = ($values | Measure-Object -Minimum).Minimum
        $maxVal = ($values | Measure-Object -Maximum).Maximum
        $minY = [Math]::Floor([Math]::Log10($minVal))
        $maxY = [Math]::Ceiling([Math]::Log10($maxVal))
    } else {
        $minY = 0.0
        $maxY = [Math]::Ceiling((($values | Measure-Object -Maximum).Maximum) * 1.12)
        if ($DrawIdealLine -and $maxY -lt 110) {
            $maxY = 110
        }
    }

    function Map-X([int]$Index) {
        return $left + $Index * $plotWidth / ($PList.Count - 1)
    }

    function Map-Y([double]$Value) {
        if ($LogScale) {
            $v = [Math]::Log10($Value)
        } else {
            $v = $Value
        }
        return $top + ($maxY - $v) * $plotHeight / ($maxY - $minY)
    }

    $graphics.DrawString($Title, $fontTitle, $black, 315, 22)
    $graphics.DrawLine($axisPen, $left, $top, $left, $top + $plotHeight)
    $graphics.DrawLine($axisPen, $left, $top + $plotHeight, $left + $plotWidth, $top + $plotHeight)

    if ($LogScale) {
        $tickValues = @()
        for ($e = [int]$minY; $e -le [int]$maxY; $e++) {
            $tickValues += [Math]::Pow(10, $e)
        }
    } else {
        $tickValues = @()
        $step = [Math]::Max(1.0, [Math]::Ceiling($maxY / 5.0 / 10.0) * 10.0)
        if ($maxY -le 20) { $step = 5.0 }
        for ($v = 0.0; $v -le $maxY + 0.0001; $v += $step) {
            $tickValues += $v
        }
    }

    foreach ($tick in $tickValues) {
        $y = Map-Y $tick
        $graphics.DrawLine($gridPen, $left, $y, $left + $plotWidth, $y)
        if ($LogScale) {
            $label = "{0:g}" -f $tick
        } else {
            $label = "{0:0}" -f $tick
        }
        $graphics.DrawString($label, $fontSmall, $black, 20, $y - 8)
    }

    for ($i = 0; $i -lt $PList.Count; $i++) {
        $x = Map-X $i
        $graphics.DrawLine($axisPen, $x, $top + $plotHeight, $x, $top + $plotHeight + 5)
        $graphics.DrawString([string]$PList[$i], $fontSmall, $black, $x - 8, $top + $plotHeight + 12)
    }

    $graphics.DrawString("Number of threads (P)", $fontAxis, $black, $left + 300, $height - 45)
    $graphics.DrawString($YLabel, $fontAxis, $black, 18, 38)

    if ($DrawIdealLine) {
        $idealPen = New-Object System.Drawing.Pen([System.Drawing.Color]::Red, 1.6)
        $idealPen.DashStyle = [System.Drawing.Drawing2D.DashStyle]::Dot
        $y100 = Map-Y 100
        $graphics.DrawLine($idealPen, $left, $y100, $left + $plotWidth, $y100)
        $idealPen.Dispose()
    }

    for ($j = 0; $j -lt $NLabels.Count; $j++) {
        $pen = New-Object System.Drawing.Pen($Colors[$j], 2.2)
        $brush = New-Object System.Drawing.SolidBrush($Colors[$j])
        for ($i = 0; $i -lt $PList.Count - 1; $i++) {
            $x1 = Map-X $i
            $y1 = Map-Y ([double]$Data[$i][$j])
            $x2 = Map-X ($i + 1)
            $y2 = Map-Y ([double]$Data[$i + 1][$j])
            $graphics.DrawLine($pen, $x1, $y1, $x2, $y2)
        }
        for ($i = 0; $i -lt $PList.Count; $i++) {
            Draw-Marker $graphics $brush (Map-X $i) (Map-Y ([double]$Data[$i][$j]))
        }
        $legendX = $left + $plotWidth + 35
        $legendY = $top + 25 + $j * 28
        $graphics.DrawLine($pen, $legendX, $legendY + 7, $legendX + 28, $legendY + 7)
        Draw-Marker $graphics $brush ($legendX + 14) ($legendY + 7)
        $graphics.DrawString("n = $($NLabels[$j])", $fontSmall, $black, $legendX + 40, $legendY)
        $pen.Dispose()
        $brush.Dispose()
    }
    $graphics.DrawString("Problem size", $fontAxis, $black, $left + $plotWidth + 35, $top - 5)

    $bitmap.Save((Join-Path (Get-Location) $FileName), [System.Drawing.Imaging.ImageFormat]::Png)

    $gridPen.Dispose()
    $axisPen.Dispose()
    $fontTitle.Dispose()
    $fontAxis.Dispose()
    $fontSmall.Dispose()
    $graphics.Dispose()
    $bitmap.Dispose()
}

$SpeedupData = Get-SpeedupData
$EfficiencyData = Get-EfficiencyData $SpeedupData

New-LineChart "execution_time.png" "PSRS Sort: Execution Time vs Threads" "Execution time (s, log scale)" $TimeData $true $false
New-LineChart "speedup.png" "PSRS Sort: Speedup vs Threads" "Speedup (S)" $SpeedupData $false $false
New-LineChart "efficiency.png" "PSRS Sort: Parallel Efficiency vs Threads" "Parallel efficiency (%)" $EfficiencyData $false $true

Write-Host "Generated: execution_time.png, speedup.png, efficiency.png"
