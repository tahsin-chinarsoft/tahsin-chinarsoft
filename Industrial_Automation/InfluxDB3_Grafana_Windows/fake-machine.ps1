# Fake machine -> InfluxDB 3 Core (HTTP line protocol), every 2 s.
# Needs: $env:INFLUXDB3_AUTH_TOKEN and a database called "factory".
$h = @{ Authorization = "Bearer $env:INFLUXDB3_AUTH_TOKEN" }
$parts = 0
$states = "RUNNING", "RUNNING", "RUNNING", "IDLE", "FAULT"
while ($true) {
    $parts += Get-Random -Max 3
    $t = [math]::Round(55 + (Get-Random -Max 100) / 10, 1)
    $s = $states | Get-Random
    $line = "machine,line=line1 temp=$t,parts=${parts}i,state=`"$s`""
    Invoke-RestMethod "http://127.0.0.1:8181/api/v3/write_lp?db=factory&precision=s" -Method Post -Headers $h -Body $line | Out-Null
    Write-Host $line
    Start-Sleep 2
}
