# This is a PowerShell plotting script. The file keeps the original name
# plot.cpp, but it should be executed with:
#   powershell -ExecutionPolicy Bypass -File .\plot.cpp

Add-Type -AssemblyName System.Drawing

$NLabels = @("1e6", "2e6", "3e6", "4e6", "5e6", "6e6", "7e6", "8e6")
$PList = @(1, 2, 4, 8, 16, 32, 64)

# Average execution time in seconds.
# Rows correspond to PList; columns correspond to NLabels.
$TimeData = @(
    @(0.1583148,  0.4329618,  0.7792232,  1.1081974,  1.3281020,  1.7566100,  1.9393160,  2.5759120),
    @(0.07051886, 0.1907816,  0.3534726,  0.5425378,  0.6906666,  0.9751436,  1.0977184,  1.3222660),
    @(0.04264138, 0.09181366, 0.1493080,  0.2167044,  0.3467912,  0.4539876,  0.5290914,  0.6736476),
    @(0.02785216, 0.0560848,  0.08404774, 0.1221296,  0.1609810,  0.2068688,  0.2452624,  0.2837836),
    @(0.02257412, 0.04551838, 0.06470564, 0.08615616, 0.1120988,  0.1312670,  0.1606434,  0.1791922),
    @(0.02063696, 0.04426982, 0.06231860, 0.08337848, 0.10229586, 0.1230348,  0.1437780,  0.1594184),
    @(0.02616302, 0.04304782, 0.06117418, 0.08274662, 0.10026986, 0.1207908,  0.1415216,  0.1666396)
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

New-LineChart "execution_time.png" "Perfect Number Search: Execution Time vs Threads" "Execution time (s, log scale)" $TimeData $true $false
New-LineChart "speedup.png" "Perfect Number Search: Speedup vs Threads" "Speedup (S)" $SpeedupData $false $false
New-LineChart "efficiency.png" "Perfect Number Search: Parallel Efficiency vs Threads" "Parallel efficiency (%)" $EfficiencyData $false $true

Write-Host "Generated: execution_time.png, speedup.png, efficiency.png"
