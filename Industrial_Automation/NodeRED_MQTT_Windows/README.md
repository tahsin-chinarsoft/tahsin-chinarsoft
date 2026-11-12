# Node-RED + Mosquitto MQTT on Windows (no Docker)

Install Node-RED and the Mosquitto MQTT broker natively on Windows, fix the **"local only mode"** trap
that stops your ESP32 or PLC gateway from connecting, and finish with a live Node-RED Dashboard 2.0.

▶ Video: [Why Can't My ESP32 Connect to Mosquitto? Node-RED + MQTT on Windows](https://youtu.be/BBT5a6pQRPU) · Short: [My MQTT broker worked... only on localhost](https://youtube.com/shorts/k87I5ppWpuo)

## Tested with

| | Version |
|---|---|
| Windows | 11 (PowerShell 5.1) |
| Node.js / npm | v24.13.1 / 11.8.0 |
| Node-RED | 5.0.9 |
| Mosquitto | 2.1.2 (winget `EclipseFoundation.Mosquitto`) |
| Dashboard | `@flowfuse/node-red-dashboard` (Dashboard 2.0) |

## 1. Node-RED

```powershell
node -v; npm -v
npm install -g node-red        # --unsafe-perm is not needed on current npm
node-red                       # keep this terminal open -> http://127.0.0.1:1880
```

## 2. Mosquitto (Administrator PowerShell!)

```powershell
winget install --id EclipseFoundation.Mosquitto -e --accept-package-agreements --accept-source-agreements
```

From a normal window the installer needs a UAC prompt. If it is cancelled you get
`0x800704c7 : The operation was canceled by the user`. Run it from an Administrator PowerShell instead.

## 3. Test on localhost

```powershell
$M = "C:\Program Files\mosquitto"
Get-Service mosquitto; Start-Service mosquitto
$s = Start-Process "$M\mosquitto_sub.exe" -ArgumentList '-h localhost -t factory/# -v -C 1' -NoNewWindow -PassThru
Start-Sleep 1; & "$M\mosquitto_pub.exe" -h localhost -t factory/test -m "hello from localhost"
```

## 4. The trap: connect with the network IP

```powershell
$IP = (Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.InterfaceAlias -notmatch 'Loopback|vEthernet|WSL|Tailscale' -and $_.IPAddress -notlike '169.*' } | Select-Object -First 1).IPAddress
"My network IP: $IP"
& "$M\mosquitto_pub.exe" -h $IP -t factory/test -m "hello from the network"
# Error: Connection refused
```

Since 2.0, Mosquitto starts in **local only mode** when `mosquitto.conf` has no `listener`.
It only listens on 127.0.0.1, so your PC works and every other device is refused.

## 5. The fix: 2 config lines + 1 firewall rule

```powershell
Add-Content "$M\mosquitto.conf" "`nlistener 1883`nallow_anonymous true`n"
Restart-Service mosquitto
New-NetFirewallRule -DisplayName "Mosquitto MQTT 1883" -Direction Inbound -Protocol TCP -LocalPort 1883 -Action Allow -Profile Private
& "$M\mosquitto_pub.exe" -h $IP -t factory/test -m "hello from the network"
```

The lines are also in [`mosquitto-listener.conf`](mosquitto-listener.conf).

> ⚠️ `allow_anonymous true` is for local development only. On a factory network use
> `mosquitto_passwd` (username + password) and TLS.

## 6. Node-RED flows

1. Node-RED → Menu → **Import** → [`flows/mqtt-lab.json`](flows/mqtt-lab.json)
   (fake machine → MQTT out `factory/line1/data`, MQTT in `factory/#` → debug).
2. Open the broker config node and set **Server** to your PC's network IP (not `localhost`).
3. Menu → **Manage palette** → Install → `@flowfuse/node-red-dashboard`.
4. Import [`flows/dashboard.json`](flows/dashboard.json) into the same tab
   (MQTT in → split fields → gauge / text / chart) and **Deploy**.
5. Open `http://127.0.0.1:1880/dashboard`.

Payload published every 2 s:

```json
{ "state": "RUNNING", "parts": 214, "temp": 64.3, "ts": 1794478105161 }
```

Replace the fake machine with a real ESP32 publishing the same JSON to `factory/line1/data`, and nothing else changes.

## Common errors

| Error | Fix |
|---|---|
| Works on the same PC, other devices can't connect | Local only mode → add `listener 1883` |
| Config is correct but still refused | Windows Firewall rule for TCP 1883 |
| Can't save `mosquitto.conf` | It's in Program Files → Administrator PowerShell |
| `node-red` command not found | Close and reopen the terminal (PATH: `%APPDATA%\npm`) |
| winget `0x800704c7` | UAC was cancelled → run as Administrator |

## Cleanup (optional)

```powershell
Stop-Service mosquitto
Remove-NetFirewallRule -DisplayName "Mosquitto MQTT 1883"
```
