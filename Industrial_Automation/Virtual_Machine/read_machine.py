#!/usr/bin/env python3
"""Read the ChinarSoft virtual machine once per second (Modbus TCP client).

  python read_machine.py                 # localhost:5020
  python read_machine.py --host 192.168.1.50 --port 502
  python read_machine.py --start         # send start command first
"""

import argparse
import time

from pymodbus.client import ModbusTcpClient

STATES = {0: "STOPPED", 1: "RUNNING", 2: "IDLE", 3: "FAULT",
          4: "CHANGEOVER", 5: "PLANNED_STOP"}
REASONS = {0: "-", 1: "material shortage", 2: "jam", 3: "motor overload",
           4: "sensor fault", 5: "changeover", 6: "break/meal",
           7: "planned maintenance"}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=5020)
    ap.add_argument("--count", type=int, default=0, help="reads (0 = forever)")
    ap.add_argument("--start", action="store_true", help="send start command")
    ap.add_argument("--reset", action="store_true", help="send fault reset")
    args = ap.parse_args()

    client = ModbusTcpClient(args.host, port=args.port)
    if not client.connect():
        raise SystemExit(f"cannot connect to {args.host}:{args.port}")

    if args.start:
        client.write_register(20, 1, device_id=1)
    if args.reset:
        client.write_register(20, 3, device_id=1)

    n = 0
    while args.count == 0 or n < args.count:
        rr = client.read_holding_registers(0, count=12, device_id=1)
        if rr.isError():
            print("read error:", rr)
        else:
            r = rr.registers
            good = (r[2] << 16) | r[3]
            scrap = (r[4] << 16) | r[5]
            print(f"{STATES.get(r[0], r[0]):<12} reason={REASONS.get(r[1], r[1]):<20}"
                  f" good={good:<6} scrap={scrap:<4} cycle={r[7] / 10:.1f}s"
                  f" (ideal {r[6] / 10:.1f}s) product={r[8]}"
                  f" temp={r[9] / 10:.1f}C hb={r[10]} in_state={r[11]}s")
        n += 1
        time.sleep(1)
    client.close()


if __name__ == "__main__":
    main()
