# ChinarSoft Virtual Machine

A packaging machine that lives in a Python script and talks **Modbus TCP**.
No PLC, no hardware, no extra libraries: run it and connect Node-RED, Python,
a SCADA or an HMI exactly as you would to a real machine.

Used in the ChinarSoft series **Build a Factory Monitoring System from Scratch**
https://www.youtube.com/@chinarsoft

## Run it

```bash
python machine_sim.py                    # port 5020, real time
python machine_sim.py --speed 60         # 1 real second = 1 machine minute
python machine_sim.py --seed 7           # same shift every time (used in the videos)
python machine_sim.py --seed 7 --hours 8 --speed 60
                                         # one 8-hour shift in 8 minutes, then the true OEE report
```

The `--hours` report is the answer key: availability, performance, quality and OEE
computed inside the machine, plus the wrong OEE you get when planned stops are
counted as downtime.

Python 3.9+ only. Port 502 needs admin rights, so the default is 5020.

Test client (needs `pip install pymodbus`):

```bash
python read_machine.py                   # print the machine once per second
python read_machine.py --start           # send START, then print
```

## What the machine does

- Runs at an ideal 2.0 s cycle (30 parts/min), sometimes slower (performance loss).
- About 2.5 % of parts are scrap (quality loss).
- **Unplanned stops:** material shortage (IDLE), jam, motor overload, sensor fault (FAULT).
- **Planned stops:** product changeover, break/meal.
- Over an 8-hour shift OEE lands around 60-72 %, like a real line.

## Register map (holding registers, function code 3)

| Address | Name | Meaning |
|---|---|---|
| 0 | machine_state | 0 STOPPED, 1 RUNNING, 2 IDLE, 3 FAULT, 4 CHANGEOVER, 5 PLANNED_STOP |
| 1 | stop_reason | 0 none, 1 material shortage, 2 jam, 3 motor overload, 4 sensor fault, 5 changeover, 6 break/meal, 7 planned maintenance |
| 2-3 | good_count | 32-bit, high word first |
| 4-5 | scrap_count | 32-bit, high word first |
| 6 | ideal_cycle_ds | ideal cycle time, 0.1 s units (20 = 2.0 s) |
| 7 | actual_cycle_ds | last cycle time, 0.1 s units |
| 8 | product_id | product currently running (101 / 102) |
| 9 | motor_temp_dc | motor temperature, 0.1 °C units |
| 10 | heartbeat | +1 every simulated second, wraps at 65535 |
| 11 | state_seconds | seconds in the current state |
| 20 | command | write 1 = start, 2 = stop, 3 = reset fault (cleared after use) |

Planned states (4, 5) must not count against availability. Everything else that
is not RUNNING is downtime. That one rule is the subject of Episode 1.

## Files

- `machine_sim.py` - the machine and a minimal Modbus TCP server (function codes 3, 6, 16)
- `read_machine.py` - simple Modbus client for testing

License: MIT
