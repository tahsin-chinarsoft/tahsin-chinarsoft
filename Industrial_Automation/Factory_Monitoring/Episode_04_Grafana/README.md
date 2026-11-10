# Episode 4 – Grafana: a live OEE dashboard

Dashboard for **Build a Factory Monitoring System from Scratch – Episode 4** (ChinarSoft).
It reads the data that the [Episode 3 flow](../Episode_03_InfluxDB) writes to InfluxDB 3 Core and
shows OEE, availability, performance, quality, a machine-state timeline, unplanned downtime by
reason, the parts counter and the stop log — all from SQL queries.

## What you need

- Everything from Episode 3 running (virtual machine → Node-RED → InfluxDB 3 Core, database `factory`)
- [Grafana OSS](https://grafana.com/grafana/download) 12.2 or newer (tested with 13.3.0)

## Connect Grafana to InfluxDB 3 Core

Connections → Data sources → Add → **InfluxDB**:

| Field | Value |
|---|---|
| URL | `http://127.0.0.1:8181` |
| Product | InfluxDB OSS 3.x |
| Query language | SQL |
| Database | `factory` |
| Token | your InfluxDB admin token |
| Advanced Database Settings → Insecure Connection | on (local, no TLS) |

**Save & test** should answer *OK*. SQL queries use Flight SQL (gRPC) on the same port.
Prefer files? See [`provisioning/influxdb-datasource.yaml`](provisioning/influxdb-datasource.yaml).

## Import the dashboard

Dashboards → New → Import → upload `dashboard_fm04.json` → pick the InfluxDB data source → Import.
Set the time range to your shift (with the simulator at `--speed 60`, a whole 8-hour shift fits in
the last ~9 minutes).

## OEE in one query

```sql
SELECT
  run_s / (run_s + unplanned_s)                          AS availability,
  2.0 * parts / run_s                                    AS performance,
  good / parts                                           AS quality,
  run_s / (run_s + unplanned_s) * 2.0 * parts / run_s * good / parts AS oee
FROM (
  SELECT
    SUM(CASE WHEN bucket = 'run'       THEN dt ELSE 0 END) * 1.0 AS run_s,
    SUM(CASE WHEN bucket = 'unplanned' THEN dt ELSE 0 END) * 1.0 AS unplanned_s,
    (MAX(good) - MIN(good) + MAX(scrap) - MIN(scrap)) * 1.0      AS parts,
    (MAX(good) - MIN(good)) * 1.0                                AS good
  FROM machine
  WHERE $__timeFilter(time)
)
```

- Planned stops (`bucket = 'planned'`) are left out of availability — the rule from Episode 1.
- `2.0` is the virtual machine's ideal cycle time in seconds. Change it for your machine.
- `$__timeFilter(time)` follows Grafana's time picker, so the same query works for a shift, a day or a week.

## Tested

Grafana 13.3.0 + InfluxDB 3 Core 3.12.2, seed 7, full 8-hour shift at `--speed 60`:

| | Grafana | Machine report |
|---|---:|---:|
| Availability | 81.6% | 81.9% |
| Performance | 89.8% | 89.3% |
| Quality | 97.4% | 97.4% |
| **OEE** | **71.4%** | **71.3%** |

The small differences come from reading the machine once per machine minute.
