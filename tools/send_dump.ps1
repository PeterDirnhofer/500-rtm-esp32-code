# Send DUMP command to a COM port. Close the serial monitor first.
param(
    [string]$Port = "COM9",
    [int]$Baud = 115200
)

$p = New-Object System.IO.Ports.SerialPort($Port, $Baud, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
try {
    $p.Open()
    $p.WriteLine('DUMP')
    Start-Sleep -Milliseconds 200
} finally {
    if ($p.IsOpen) { $p.Close() }
}
Write-Output "Sent DUMP to $Port at $Baud bps"
