# Episode 2 – Node-RED: read the machine and catch every stop

Flow for **Build a Factory Monitoring System from Scratch – Episode 2** (ChinarSoft).
It reads the [ChinarSoft virtual machine](../../Virtual_Machine) over Modbus TCP once per
second, decodes the registers, detects every stop, shows a live dashboard and appends each
finished stop to `stops.csv`.

## What you need

- Node.js 18 or newer
- Node-RED 4 (`npm install -g --unsafe-perm node-red`)
- Two palette nodes (Menu → Manage palette → Install):
  - `node-red-contrib-modbus`
  - `@flowfuse/node-red-dashboard`

## Run it

```bash
python3 machine_sim.py --seed 7 --speed 60     # terminal 1: the virtual machine
node-red                                       # terminal 2
```

1. Open http://127.0.0.1:1880, Menu → Import → select `flow_fm02.json` → Import → **Deploy**.
2. Open the dashboard: http://127.0.0.1:1880/dashboard/machine

## How it works

| Node | Job |
|---|---|
| Read machine | Modbus TCP, function code 3, holding registers 0–11, every 1 s |
| Decode registers | State and reason as text, 32-bit counters (high word first), 0.1 scaling |
| Detect stops | Compares each reading with the previous one; when the machine leaves a non-running state, one stop record is created. CHANGEOVER and PLANNED_STOP are **planned**, everything else is **unplanned** |
| Status card / Stop log | Dashboard 2.0 widgets |
| To CSV line → file | One line per stop in `stops.csv` (set `STOPS_FILE` to change the path) |

Durations use the machine's own clock (heartbeat register, +1 per machine second), so they
stay correct even when the simulator runs at `--speed 60`. Node-RED can only count what it
saw: start it before the shift for complete totals.
