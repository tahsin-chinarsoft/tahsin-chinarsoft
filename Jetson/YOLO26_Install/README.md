# YOLO26 on Jetson Orin Nano (JetPack 7.2) + TensorRT

ChinarSoft YouTube video: install Ultralytics YOLO26 on a Jetson Orin Nano Super running JetPack 7.2, from an empty terminal to TensorRT. Recorded during a real first install, including every error.

▶ Video: [YOLO26 on Jetson Orin Nano (JetPack 7.2): 23 ms → 3.5 ms with TensorRT](https://youtu.be/0UxXNWHBBHg) · Short: [CUDA True, wrong PyTorch](https://youtube.com/shorts/rElUG5Ex-cA)

## Results

YOLO26n, 640×640, `bus.jpg`, 10 warm-up runs then 100 timed runs, inference time only (`benchmark.py`):

| Model | Avg inference | FPS |
| --- | --- | --- |
| PyTorch (`yolo26n.pt`) | 23.09 ms | ~43 |
| TensorRT FP16 (`yolo26n.engine`) | **3.55 ms** | **~281** |

About **6.5× faster**. On a video (596 frames) the TensorRT engine averaged 3.6 ms inference per frame.

## Tested on

| | |
| --- | --- |
| Board | Jetson Orin Nano Super Developer Kit 8 GB |
| JetPack | 7.2 (L4T R39.2.1), Ubuntu 24.04.5 |
| Python / CUDA | 3.12.3 / 13.2 |
| Power mode | MAXN_SUPER + `jetson_clocks` |
| PyTorch | 2.15.0+cu132, torchvision 0.29.2+cu132 |
| Ultralytics / TensorRT | 8.4.211 / 10.16.1.11 |

## Install

Run the steps one by one, or use [`install.sh`](install.sh).

```bash
# 1. Check the system
cat /etc/nv_tegra_release; python3 --version; ls /usr/local/ | grep cuda

# 2. Max power: LIST the modes first (on this board mode 0 is 15W, MAXN_SUPER is 2)
sudo nvpmodel -q; grep "POWER_MODEL ID" /etc/nvpmodel.conf
sudo nvpmodel -m 2          # asks for a reboot
# after the reboot:
sudo jetson_clocks && sudo nvpmodel -q

# 3. Virtual environment (Ubuntu 24.04 blocks pip on the system Python)
sudo apt update && sudo apt install -y python3-pip python3-venv
python3 -m venv ~/yolo-env --system-site-packages   # sees JetPack's TensorRT
source ~/yolo-env/bin/activate
pip install -U pip

# 4. PyTorch for Orin: the cu132 index, BEFORE ultralytics
pip install --pre torch torchvision --extra-index-url https://download.pytorch.org/whl/cu132
python -c "import torch; print(torch.__version__, torch.cuda.get_device_capability(0)); x=torch.rand(1000,1000,device='cuda'); print((x@x).sum().item())"
# expect: 2.15.0+cu132 (8, 7) <number>, and NO warning

# 5. Ultralytics, then a fresh scipy inside the venv
pip install ultralytics
pip install -U scipy
python -c "import torch, ultralytics; print(torch.__version__, ultralytics.__version__)"   # torch must still be +cu132

# 6. First detection
yolo predict model=yolo26n.pt source='https://ultralytics.com/images/bus.jpg' device=0

# 7. Export to TensorRT FP16 (about 7 minutes on Orin Nano)
python -c "import tensorrt; print(tensorrt.__version__)"
yolo export model=yolo26n.pt format=engine half=True device=0

# 8. Compare
python benchmark.py
```

## Common errors

| Error | Fix |
| --- | --- |
| `error: externally-managed-environment` | Use a virtual environment (step 3). |
| `Found GPU0 Orin which is of compute capability (CC) 8.7 ... No published PyTorch CUDA builds ... support this GPU` | You installed the generic cu130 build. It still prints `cuda.is_available() True`, but it is not built for Orin (sm_87). Uninstall it and install from the cu132 index (step 4). |
| `scipy ... requires numpy<1.28` | The venv sees the old system scipy. Run `pip install -U scipy` inside the venv. |
| `No module named tensorrt` | Create the venv with `--system-site-packages`. |
| SSH times out after `nvpmodel -m 2` reboot (Tailscale) | Wait, or run `sudo systemctl enable tailscaled` once so it starts at boot. |
| Export looks frozen | Building the engine takes several minutes. Check that `jetson_clocks` is on. |

Notes:

- Don't trust `torch.cuda.is_available()` alone. Check `torch.cuda.get_device_capability(0)` is `(8, 7)` and that no warning is printed.
- You don't need to install `onnxruntime-gpu` yourself. The 1.24.0 wheel suggested for JetPack 7 is built for SBSA (sm_90), not Orin. During `yolo export ... format=engine`, Ultralytics auto-installs what it needs (onnx, onnxslim, onnxruntime-gpu, tensorrt-cu13); in our test the export and TensorRT inference worked with those.
- A TensorRT engine belongs to the device that built it. Rebuild it on every Jetson.

## Files

| File | What it does |
| --- | --- |
| [`install.sh`](install.sh) | Steps 3–7 in one script (run after setting the power mode) |
| [`benchmark.py`](benchmark.py) | PyTorch vs TensorRT, 10 warm-up + 100 timed runs |

## Sources

- [Ultralytics: NVIDIA Jetson guide](https://docs.ultralytics.com/guides/nvidia-jetson)
- [JetPack 7.2 Orin install notes (cu132 PyTorch)](https://github.com/iuliaferoli/jetson-jp7.2-install)
- Test video: [Intel IoT DevKit sample-videos](https://github.com/intel-iot-devkit/sample-videos) (`people-detection.mp4`)

Previous video: [Jetson Orin Nano Setup Guide: NVMe SSD + JetPack](https://youtu.be/qcuPX3qRaas)
