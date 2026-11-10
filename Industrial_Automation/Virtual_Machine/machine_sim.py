#!/usr/bin/env python3
"""
ChinarSoft Virtual Machine - a packaging machine that talks Modbus TCP.

No PLC, no hardware: run this script and any Modbus client (Node-RED,
Python, a SCADA, an HMI) can read it like a real machine.

Series: "Build a Factory Monitoring System from Scratch" - ChinarSoft
https://www.youtube.com/@chinarsoft

Holding registers (function code 3, address offset 0):
  HR0   machine_state      0=STOPPED 1=RUNNING 2=IDLE 3=FAULT
                           4=CHANGEOVER 5=PLANNED_STOP
  HR1   stop_reason        0=none 1=material shortage 2=jam
                           3=motor overload 4=sensor fault
                           5=changeover 6=break/meal 7=planned maintenance
  HR2-3 good_count         32-bit, high word first
  HR4-5 scrap_count        32-bit, high word first
  HR6   ideal_cycle_ds     ideal cycle time in 0.1 s  (e.g. 20 = 2.0 s)
  HR7   actual_cycle_ds    last measured cycle time in 0.1 s
  HR8   product_id         product currently running
  HR9   motor_temp_dc      motor temperature in 0.1 degC
  HR10  heartbeat          +1 every simulated second (link check)
  HR11  state_seconds      seconds spent in the current state

Writable command registers:
  HR20  command            1=start 2=stop 3=reset fault (cleared after use)

Only the Python standard library is needed (Python 3.9+).
Supported Modbus functions: 03 read holding, 06 write single, 16 write multiple.

Usage:
  python machine_sim.py                  # port 5020, real time
  python machine_sim.py --speed 10       # 10x faster (good for demos)
  python machine_sim.py --seed 7 --hours 8 --speed 60
                                         # one shift in 8 minutes + true OEE report
"""

import argparse
import asyncio
import random
import time

import struct

STATES = {0: "STOPPED", 1: "RUNNING", 2: "IDLE", 3: "FAULT",
          4: "CHANGEOVER", 5: "PLANNED_STOP"}
REASONS = {0: "-", 1: "material shortage", 2: "jam", 3: "motor overload",
           4: "sensor fault", 5: "changeover", 6: "break/meal",
           7: "planned maintenance"}

# Unplanned events while running: (state, reason, probability per second,
# min duration s, max duration s)
UNPLANNED = [
    (2, 1, 1 / 1800, 60, 300),    # idle: waiting for material
    (3, 2, 1 / 1200, 30, 180),    # fault: jam
    (3, 3, 1 / 7200, 300, 900),   # fault: motor overload
    (3, 4, 1 / 5400, 60, 240),    # fault: sensor fault
]
# Planned events: (state, reason, probability per second, min s, max s)
PLANNED = [
    (4, 5, 1 / 5400, 600, 1200),  # changeover to another product
    (5, 6, 1 / 7200, 900, 1800),  # break / meal
]

IDEAL_CYCLE_DS = 20     # 2.0 s ideal cycle -> 30 parts/min
SCRAP_RATE = 0.025      # 2.5 % scrap while running


class Machine:
    def __init__(self, rng: random.Random):
        self.rng = rng
        self.state = 1
        self.reason = 0
        self.good = 0
        self.scrap = 0
        self.product = 101
        self.temp = 350.0          # 0.1 degC -> 35.0 degC
        self.heartbeat = 0
        self.state_left = 0.0      # seconds until a timed stop ends
        self.state_since = 0.0
        self.cycle_acc = 0.0
        self.last_cycle = IDEAL_CYCLE_DS
        self.seconds_in = {k: 0 for k in STATES}   # time per state (truth)

    def set_state(self, state, reason, duration, now):
        self.state, self.reason = state, reason
        self.state_left = duration
        self.state_since = now

    def command(self, cmd, now):
        if cmd == 1 and self.state in (0, 2):
            self.set_state(1, 0, 0, now)
        elif cmd == 2:
            self.set_state(0, 0, 0, now)
        elif cmd == 3 and self.state == 3:
            self.set_state(1, 0, 0, now)

    def step(self, dt, now):
        """Advance the machine by dt simulated seconds."""
        self.heartbeat = (self.heartbeat + 1) % 65536
        self.seconds_in[self.state] += dt

        if self.state == 1:  # RUNNING
            # slower than ideal sometimes -> performance loss
            cycle = IDEAL_CYCLE_DS * self.rng.uniform(1.0, 1.25)
            self.cycle_acc += dt * 10 / cycle
            while self.cycle_acc >= 1:
                self.cycle_acc -= 1
                if self.rng.random() < SCRAP_RATE:
                    self.scrap += 1
                else:
                    self.good += 1
            self.last_cycle = round(cycle)
            self.temp = min(self.temp + dt * 0.05, 650)

            for state, reason, p, lo, hi in UNPLANNED + PLANNED:
                if self.rng.random() < p * dt:
                    self.set_state(state, reason, self.rng.uniform(lo, hi), now)
                    if reason == 5:  # changeover -> new product
                        self.product = 101 if self.product != 101 else 102
                    break
        else:
            self.temp = max(self.temp - dt * 0.08, 250)
            if self.state_left > 0:
                self.state_left -= dt
                if self.state_left <= 0:
                    self.set_state(1, 0, 0, now)

    def report(self):
        """True shift figures, computed inside the machine (the answer key)."""
        t = self.seconds_in
        total_time = sum(t.values())
        planned = t[4] + t[5]
        planned_prod = total_time - planned
        run = t[1]
        parts = self.good + self.scrap
        a = run / planned_prod if planned_prod else 0
        p = parts * IDEAL_CYCLE_DS / 10 / run if run else 0
        q = self.good / parts if parts else 0
        a_wrong = run / total_time if total_time else 0
        m = lambda s: f"{s / 60:6.1f} min"
        return "\n".join([
            "", "=" * 52,
            f" SHIFT REPORT ({total_time / 3600:.1f} h machine time)",
            "=" * 52,
            f" Running            {m(run)}",
            f" Planned stops      {m(planned)}  (changeover, breaks)",
            f" Unplanned stops    {m(total_time - run - planned)}",
            f" Good / scrap       {self.good} / {self.scrap}",
            "-" * 52,
            f" Availability       {a:6.1%}",
            f" Performance        {p:6.1%}",
            f" Quality            {q:6.1%}",
            f" OEE                {a * p * q:6.1%}",
            f" OEE if planned stops counted as downtime: {a_wrong * p * q:.1%}",
            "=" * 52,
        ])

    def registers(self, now):
        return [
            self.state, self.reason,
            (self.good >> 16) & 0xFFFF, self.good & 0xFFFF,
            (self.scrap >> 16) & 0xFFFF, self.scrap & 0xFFFF,
            IDEAL_CYCLE_DS, self.last_cycle, self.product,
            int(self.temp), self.heartbeat,
            min(int(now - self.state_since), 65535),
        ]


class ModbusTcpServer:
    """Minimal Modbus TCP server: one register table, function codes 3/6/16."""

    def __init__(self, size=100):
        self.hr = [0] * size

    async def handle(self, reader, writer):
        try:
            while True:
                # MBAP header: transaction id, protocol id, length, unit id
                header = await reader.readexactly(7)
                tid, pid, length, unit = struct.unpack(">HHHB", header)
                pdu = await reader.readexactly(length - 1)
                reply = self.process(pdu)
                writer.write(struct.pack(">HHHB", tid, pid, len(reply) + 1, unit) + reply)
                await writer.drain()
        except (asyncio.IncompleteReadError, ConnectionError):
            pass
        finally:
            writer.close()

    def process(self, pdu):
        fc = pdu[0]
        try:
            if fc == 3:                                   # read holding registers
                addr, count = struct.unpack(">HH", pdu[1:5])
                if not 1 <= count <= 125 or addr + count > len(self.hr):
                    return bytes([fc | 0x80, 2])          # illegal data address
                data = self.hr[addr:addr + count]
                return bytes([fc, count * 2]) + struct.pack(f">{count}H", *data)
            if fc == 6:                                   # write single register
                addr, value = struct.unpack(">HH", pdu[1:5])
                if addr >= len(self.hr):
                    return bytes([fc | 0x80, 2])
                self.hr[addr] = value
                return pdu[:5]                            # echo request
            if fc == 16:                                  # write multiple registers
                addr, count = struct.unpack(">HH", pdu[1:5])
                if addr + count > len(self.hr):
                    return bytes([fc | 0x80, 2])
                self.hr[addr:addr + count] = struct.unpack(f">{count}H", pdu[6:6 + count * 2])
                return pdu[:5]
            return bytes([fc | 0x80, 1])                  # illegal function
        except struct.error:
            return bytes([fc | 0x80, 3])                  # illegal data value


async def run_machine(machine, server, speed, hours):
    sim_time = 0.0
    end = hours * 3600 if hours else None
    while end is None or sim_time < end:
        await asyncio.sleep(1)
        cmd = server.hr[20]
        if cmd:
            machine.command(cmd, sim_time)
            server.hr[20] = 0
        for _ in range(max(1, int(speed))):
            if end is not None and sim_time >= end:
                break
            sim_time += 1
            machine.step(1, sim_time)
        server.hr[0:12] = machine.registers(sim_time)
        total = machine.good + machine.scrap
        print(f"\r[{time.strftime('%H:%M:%S')}] "
              f"{STATES[machine.state]:<12} "
              f"reason: {REASONS[machine.reason]:<20} "
              f"good: {machine.good:>6}  scrap: {machine.scrap:>4}  "
              f"total: {total:>6}  temp: {machine.temp / 10:4.1f} C   ",
              end="", flush=True)
    print(machine.report())


async def main():
    ap = argparse.ArgumentParser(description="ChinarSoft virtual machine (Modbus TCP)")
    ap.add_argument("--host", default="0.0.0.0")
    ap.add_argument("--port", type=int, default=5020)
    ap.add_argument("--speed", type=float, default=1,
                    help="simulated seconds per real second (default 1)")
    ap.add_argument("--seed", type=int, default=None,
                    help="random seed for a repeatable run")
    ap.add_argument("--hours", type=float, default=None,
                    help="stop after this much machine time and print the true shift report")
    args = ap.parse_args()

    machine = Machine(random.Random(args.seed))
    server = ModbusTcpServer()
    server.hr[0:12] = machine.registers(0)
    tcp = await asyncio.start_server(server.handle, args.host, args.port)
    print(f"ChinarSoft virtual machine on {args.host}:{args.port} "
          f"(speed x{args.speed:g}) - Ctrl+C to stop")
    async with tcp:
        await run_machine(machine, server, args.speed, args.hours)
        if args.hours is None:
            await tcp.serve_forever()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nstopped")
