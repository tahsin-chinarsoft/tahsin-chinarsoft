# Run once in an Administrator PowerShell: starts InfluxDB 3 Core at Windows startup.
$a = New-ScheduledTaskAction -Execute "C:\influxdb3\influxdb3.exe" `
     -Argument "serve --node-id win01 --object-store file --data-dir C:\influxdb3\data --http-bind 127.0.0.1:8181"
Register-ScheduledTask -TaskName "InfluxDB3" -Action $a -Trigger (New-ScheduledTaskTrigger -AtStartup) -User "SYSTEM" -RunLevel Highest
# Remove: Unregister-ScheduledTask -TaskName InfluxDB3 -Confirm:$false
