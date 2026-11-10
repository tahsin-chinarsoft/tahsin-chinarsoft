# Episode 3 – InfluxDB: give the data a memory

Flow for **Build a Factory Monitoring System from Scratch – Episode 3** (ChinarSoft).
It is the [Episode 2 flow](../Episode_02_NodeRED) plus InfluxDB 3 Core: every reading and
every finished stop is stored, and when Node-RED starts it rebuilds the dashboard totals and
the stop log from the database — so a restart no longer resets the shift.

## What you need

- Everything from Episode 2 (Node.js 18+, Node-RED 4, `node-red-contrib-modbus`,
  `@flowfuse/node-red-dashboard`)
- [InfluxDB 3 Core](https://docs.influxdata.com/influxdb3/core/) (free, open source). No extra Node-RED nodes: the
  flow uses the built-in HTTP request node.

## Set up InfluxDB 3 Core

```bash
curl -O https://www.influxdata.com/d/install_influxdb3.sh && sh install_influxdb3.sh
influxdb3 serve --node-id host01 --object-store file --data-dir ~/.influxdb3   # terminal 1
influxdb3 create token --admin                       # copy the token, it is shown only once
influxdb3 create database factory --token <TOKEN>
```

## Run it

```bash
python3 machine_sim.py --seed 7 --speed 60          # terminal 2: the virtual machine
INFLUX_TOKEN=<TOKEN> node-red                        # terminal 3
```

1. Open http://127.0.0.1:1880, Menu → Import → `flow_fm03.json` → Import → **Deploy**.
2. Dashboard: http://127.0.0.1:1880/dashboard/machine
3. Optional: `INFLUX_URL` if InfluxDB is not on `http://127.0.0.1:8181`.

## What gets stored

Line protocol over HTTP (`POST /api/v3/write_lp?db=factory&precision=ms`):

```
machine,line=packaging,bucket=run state="RUNNING",reason="-",good=7595i,scrap=200i,cycle=2.1,temp=26.4,dt=60i,heartbeat=17280i 1794303000000
stops,line=packaging,type=unplanned,state=IDLE,reason=material\ shortage minutes=2.4,started="00:22",ended="00:24" 1794303000000
```

- **Tags** (group by): `line`, `bucket`, `type`, `state`, `reason`
- **Fields** (values): counters, cycle time, motor temperature, `dt`, `minutes`
- `dt` = machine seconds since the previous reading, tagged with the bucket they belong to
  (`run` / `planned` / `unplanned`). `SUM(dt)` gives the exact time per bucket at any simulator speed.

## Ask questions with SQL

```sql
SELECT bucket, SUM(dt) / 60.0 AS minutes
FROM machine WHERE time > now() - INTERVAL '8 hours' GROUP BY bucket;

SELECT type, reason, COUNT(*) AS stops, SUM(minutes) AS minutes
FROM stops WHERE time > now() - INTERVAL '8 hours' GROUP BY type, reason ORDER BY minutes DESC;
```

```bash
influxdb3 query --database factory --token <TOKEN> "SELECT bucket, SUM(dt)/60.0 AS minutes FROM machine GROUP BY bucket"
```

## How the restart works

At start, *What does InfluxDB know?* runs three SQL queries (totals per bucket, the last
reading, the last 50 stops). *Detect stops* waits for the answer, then:

- the current state's share of the gap comes from the machine (`inState` register),
- for the rest of the gap, the part counter tells how much was running
  (parts made × cycle time); what's left is counted as a stop of the last known type.

Stops that start **and** end while Node-RED is down still count as time, but can't appear
as rows in the log — nobody saw them.

## Tested

InfluxDB 3 Core 3.12.2, seed 7, full 8-hour shift at `--speed 60`, Node-RED stopped for
30 machine minutes in the middle and restarted:

| | InfluxDB | Machine report |
|---|---:|---:|
| Running | 288.2 min | 291.1 min |
| Planned stops | 126.2 min | 124.8 min |
| Unplanned stops | 64.2 min | 64.2 min |
| Whole shift | 478.6 min | 480.0 min |
