# Ep. 07 — Real OEE Dashboard: Node-RED + InfluxDB 3 + Grafana

- ▶ Video: https://youtu.be/EqHhkhfun40
- 🔌 Run it in your browser (Wokwi): https://wokwi.com/projects/480371604684952577

## Files
- `sketch.ino`, `diagram.json`, `libraries.txt`: the Wokwi project. Create a new ESP32 project on wokwi.com and paste them in, or just open the link above.
- `flow_wk07.json`: Node-RED flow (MQTT in → counter-delta adapter → InfluxDB 3 line protocol). Import it, then set your device ID and InfluxDB host.
- `dashboard_wk07.json`: Grafana dashboard (InfluxDB 3 SQL datasource): OEE, A/P/Q, state timeline, parts and cycle time.

## Homework
Two machines: two Wokwi tabs, one Node-RED adapter, a Grafana $device variable

✅ Solution: https://wokwi.com/projects/480378459658771457 — files in `homework_solution/`.
(The firmware is unchanged: open the solution in TWO browser tabs, each gets its own device ID. `flow_wk07_hw.json` subscribes to both, `dashboard_wk07_hw.json` adds the `$device` variable and a 'Compare machines' table.)
