"""PyTorch vs TensorRT speed test for YOLO26 on Jetson.

Run inside the venv, after exporting yolo26n.engine:
    python benchmark.py
"""

from pathlib import Path

from ultralytics import YOLO

IMAGE = "bus.jpg"
MODELS = ["yolo26n.pt", "yolo26n.engine"]
WARMUP = 10
RUNS = 100

if not Path(IMAGE).exists():
    IMAGE = "https://ultralytics.com/images/bus.jpg"

for name in MODELS:
    if name.endswith(".engine") and not Path(name).exists():
        print(f"{name} not found. Export it first: yolo export model=yolo26n.pt format=engine half=True device=0")
        continue
    model = YOLO(name, task="detect")
    for _ in range(WARMUP):
        model(IMAGE, device=0, verbose=False)
    times = [model(IMAGE, device=0, verbose=False)[0].speed["inference"] for _ in range(RUNS)]
    avg = sum(times) / len(times)
    print(f"{name:16s} avg inference: {avg:6.2f} ms  (~{1000 / avg:.0f} FPS)")
