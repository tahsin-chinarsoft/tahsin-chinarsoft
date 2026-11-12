# InfluxDB 3 Core + Grafana on Windows (no Docker)

Install InfluxDB 3 Core and Grafana natively on Windows, get past the three Windows traps,
and finish with a live machine dashboard.

▶ Video: coming soon on [ChinarSoft](https://www.youtube.com/@chinarsoft)

## Tested with

| | Version |
|---|---|
| Windows | 11 (PowerShell 5.1) |
| InfluxDB 3 Core | 3.12.2 (Windows zip, no installer) |
| Grafana OSS | 13.3.1 (winget `GrafanaLabs.Grafana.OSS`, runs as a Windows service) |

## 1. Download InfluxDB 3 Core

```powershell
New-Item -ItemType Directory C:\influxdb3 -Force | Out-Null; cd C:\influxdb3
Invoke-WebRequest "https://dl.influxdata.com/influxdb/releases/influxdb3-core-3.12.2-windows_amd64.zip" -OutFile influxdb3-core.zip
Expand-Archive influxdb3-core.zip -DestinationPath . -Force
.\influxdb3.exe --version
```

## 2. Start the server (keep this window open)

```powershell
.\influxdb3.exe serve --node-id win01 --object-store file --data-dir C:\influxdb3\data --http-bind 127.0.0.1:8181
```

`127.0.0.1` keeps it local until you have a token (official advice) and avoids the firewall prompt.

## Trap 1: `localhost:8181` says "the request was not authenticated"

That's normal. InfluxDB 3 Core has **no web UI**. The server is running; use the CLI, the HTTP API or Grafana, with a token.

## 3. Admin token (second terminal)

```powershell
cd C:\influxdb3
.\influxdb3.exe create token --admin      # shown only once
```

## Trap 2: `export` doesn't exist in PowerShell

```powershell
$env:INFLUXDB3_AUTH_TOKEN = (Read-Host "Paste your token").Trim()
[Environment]::SetEnvironmentVariable("INFLUXDB3_AUTH_TOKEN", $env:INFLUXDB3_AUTH_TOKEN, "User")
.\influxdb3.exe show databases
```

## 4. Test

```powershell
.\influxdb3.exe create database factory
.\influxdb3.exe write --database factory 'machine,line=line1 temp=58.4,parts=12i'
.\influxdb3.exe query --database factory "SELECT * FROM machine"
Invoke-RestMethod http://127.0.0.1:8181/health -Headers @{Authorization = "Bearer $env:INFLUXDB3_AUTH_TOKEN"}
```

> PowerShell 5.1 drops inner double quotes when calling an .exe, so keep string fields out of CLI writes (or use the HTTP API, as in `fake-machine.ps1`).

## 5. Grafana

```powershell
winget install --id GrafanaLabs.Grafana.OSS -e --accept-package-agreements --accept-source-agreements
Get-Service Grafana*
```

`http://localhost:3000` → admin / admin → set a new password.

Connections → Add new connection → **InfluxDB**:
- Query language: **SQL** (13.3.1 starts on InfluxQL)
- URL: `http://127.0.0.1:8181` · Database: `factory` · Token: your token

## Trap 3: Save & test → "tls: first record does not look like a TLS handshake"

With SQL, Grafana uses Flight SQL (gRPC) and expects TLS. The local server has none → turn on **Insecure Connection** → Save & test → OK.

## 6. Live data

- [`fake-machine.ps1`](fake-machine.ps1): writes temp / parts / state every 2 s over HTTP (used in the video).
- [`nodered-to-influxdb.json`](nodered-to-influxdb.json): the same with Node-RED, reading the MQTT fake machine from [NodeRED_MQTT_Windows](../NodeRED_MQTT_Windows) (start Node-RED after setting the token variable).

## 7. Dashboard

Import [`grafana-dashboard.json`](grafana-dashboard.json) (pick your InfluxDB data source), or add a Time series panel with:

```sql
SELECT time, temp FROM machine WHERE $__timeFilter(time) ORDER BY time
```

Last 15 minutes, refresh 5s.

## Trap 4: the database stops when you close the window

InfluxDB 3 Core is not a Windows service. Run [`start-at-boot.ps1`](start-at-boot.ps1) once (Administrator) to start it at boot.
Other devices need to write? Use `--http-bind 0.0.0.0:8181` and allow TCP 8181 in the firewall.

## Common errors

| Error | Fix |
|---|---|
| "the request was not authenticated" in the browser | Normal: no UI. Use a token |
| `export` not recognized / MissingToken | `$env:INFLUXDB3_AUTH_TOKEN = "..."` or `--token` |
| Grafana: TLS handshake failed | Query language SQL + **Insecure Connection** |
| Data stops after closing the terminal | Scheduled Task (`start-at-boot.ps1`) |
| Other devices can't write | `--http-bind 0.0.0.0:8181` + firewall TCP 8181 |

Want the full system with OEE and downtime reasons? See [Factory Monitoring](../Factory_Monitoring).
