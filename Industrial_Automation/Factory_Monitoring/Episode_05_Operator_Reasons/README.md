# Episode 5 – Downtime reasons from the operator

Flow and dashboard for **Build a Factory Monitoring System from Scratch – Episode 5** (ChinarSoft).

The machine knows *that* it was idle (the infeed ran empty), not *why*. This episode adds a
tablet page to the Node-RED dashboard: every finished idle stop becomes a card with five big
buttons, the operator taps one, and the answer is written to **the same row** in InfluxDB.

## What's new compared with Episode 3

| Node | What it does |
|---|---|
| `Needs a reason?` | picks the finished IDLE stops and puts them on the tablet |
| `Reason buttons` (ui-template) | the tablet page `/dashboard/operator` — one card per open stop, five buttons |
| `Operator answer → same row` | turns the tap into line protocol with the stop's own tags and timestamp |
| `Stop → line protocol` | now uses the stop's own timestamp and writes `op_reason=""` |
| `What does InfluxDB know?` / `Restore …` | on start, idle stops with an empty `op_reason` go back on the tablet |

## The trick: same table + same tags + same timestamp = same row

```
stops,line=packaging,type=unplanned,state=IDLE,reason=material\ shortage minutes=3,started="02:08",ended="02:11",op_reason="" 1794317977907
stops,line=packaging,type=unplanned,state=IDLE,reason=material\ shortage op_reason="no operator",operator="tablet" 1794317977907
```

InfluxDB 3 keeps one row and adds the new fields (`op_reason`, `operator`). No update query, no IDs,
no second table.

## Run it

1. Start the virtual machine: `python3 machine_sim.py --seed 7 --speed 60`
2. Start InfluxDB 3 Core (see [Episode 3](../Episode_03_InfluxDB)) and set `INFLUX_TOKEN` for Node-RED
3. Import `flow_fm05.json` into Node-RED 4 (palettes: `node-red-contrib-modbus`, `@flowfuse/node-red-dashboard`) and deploy
4. Open the tablet page: `http://<node-red-host>:1880/dashboard/operator`
5. Import `dashboard_fm05.json` into Grafana (see [Episode 4](../Episode_04_Grafana) for the data source)

The reason list lives in the `Reason buttons` node (`reasons: [...]`). Let production own it.

## Grafana: operator reason first

```sql
SELECT
  CASE
    WHEN op_reason <> ''  THEN op_reason
    WHEN state = 'IDLE'   THEN 'idle – waiting for operator'
    ELSE reason
  END AS reason,
  ROUND(SUM(minutes), 1) AS minutes
FROM stops
WHERE type = 'unplanned' AND $__timeFilter(time)
GROUP BY 1
ORDER BY minutes DESC
```

A stat panel counts the idle stops still waiting for an answer.

## Tested

Node-RED 4 + InfluxDB 3 Core 3.12.2 + Grafana 13.3.0, seed 7, full 8-hour shift at `--speed 60`.
A script played the operator on the real tablet page (answer 15 s after the card appears):

| | |
|---|---|
| Idle stops | 8 (26 min), all answered |
| Rows in `stops` | 32 for 32 stops (no duplicates) |
| Unplanned minutes by reason | jam 26 · no material from warehouse 9 · sensor fault 9 · no operator 7 · wrong material 7 · quality hold 3 |
| OEE | 71.4% (unchanged — a reason changes the why, not the minutes) |
| Restart | 3 open stops before a Node-RED restart, 3 cards after it |

## Homework

Add a comment box for jams: the operator adds a note (`op_comment`) to a fault that already has a
machine reason — same row, same trick. Post your line protocol under the video.
