# Episode 6 – A long-stop alert and an automatic shift report

Flow and dashboard for **Build a Factory Monitoring System from Scratch – Episode 6** (ChinarSoft).

A dashboard only helps someone who is looking at it. This episode makes the system speak first:

- **Long-stop alert** — an unplanned stop that runs longer than `ALERT_MIN` machine minutes (default 5)
  raises one alert: a toast and a red banner on the Node-RED dashboard, and a row in the `alerts` table.
  When the stop ends, the **same row** gets `status="cleared"` and its total minutes (same tags + same
  timestamp, the trick from [Episode 5](../Episode_05_Operator_Reasons)).
- **Shift report** — when the machine clock reaches `SHIFT_HOURS` (default 8), Node-RED runs five SQL
  queries over the shift and builds one report: the page `/dashboard/report`, an HTML file in
  `REPORT_DIR`, and a row in `shift_reports` that Grafana shows shift after shift. There is also a
  *Build report now* button.

No external services: everything stays on the dashboard and in Grafana.

## What's new compared with Episode 5

| Node | What it does |
|---|---|
| `Long stop?` | on every reading: unplanned stop longer than `ALERT_MIN`? → one alert per stop |
| `Alert cleared?` | when that stop ends → same alert row, `status="cleared"`, `total_minutes` |
| `Toast` (ui-notification) · `Alert banner` (ui-template) | the alert on the dashboard |
| `Shift over?` | machine clock ≥ `SHIFT_HOURS` → start the report once |
| `Report queries` → `Query InfluxDB (report)` → `Build shift report` | 5 queries → one report |
| `Report card` · `Save HTML report` | `/dashboard/report` and the HTML file |
| `Detect stops` | now also passes the machine clock, the current stop's start and its bucket |

## Run it

1. Virtual machine: `python3 machine_sim.py --seed 7 --speed 60 --hours 8.1` (a little over 8 h, so the clock passes 08:00)
2. InfluxDB 3 Core (see [Episode 3](../Episode_03_InfluxDB)); env for Node-RED: `INFLUX_TOKEN`, optional `ALERT_MIN`, `SHIFT_HOURS`, `REPORT_DIR`
3. Import `flow_fm06.json` into Node-RED 4 (palettes: `node-red-contrib-modbus`, `@flowfuse/node-red-dashboard`) and deploy
4. Pages: `/dashboard/machine` (alerts), `/dashboard/operator` (reasons), `/dashboard/report` (shift report)
5. Import `dashboard_fm06.json` into Grafana (data source: see [Episode 4](../Episode_04_Grafana))

**Real factory:** trigger the report by the wall clock instead of the machine clock — an inject node at
your shift changes (e.g. 06:00, 14:00, 22:00) wired into `Report queries`. The queries stay the same.

## Tested

Node-RED 4 + InfluxDB 3 Core 3.12.2 + Grafana 13.3.0, seed 7 at `--speed 60`, operator answers from a script on the real tablet page:

| | Report (Node-RED, 08:00) | Machine's own report (8 h) |
|---|---|---|
| Availability | 81.8% | 81.9% |
| Performance | 89.8% | 89.3% |
| Quality | 97.4% | 97.4% |
| **OEE** | **71.6%** | **71.3%** |

- Alerts in the shift: 1 — idle stop at 03:19, raised at 03:24, cleared at 03:26 (7 min). 25 unplanned stops, only one over 5 minutes.
- Top reasons: jam 26 min · no material from warehouse 9 · sensor fault 9. Operator answered 8 of 8 idle stops.
- A motor overload that started at 07:56 raised a second alert at 08:01 — after the report, so it belongs to the next shift.
- Example output: [`example_shift_report.html`](example_shift_report.html)

## Homework

Give each reason its own limit (a 3-minute jam is already a problem, an idle machine maybe after 10).
Post your rule under the video.
