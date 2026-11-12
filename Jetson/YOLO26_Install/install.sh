#!/usr/bin/env bash
# YOLO26 + TensorRT on Jetson Orin (JetPack 7.2, CUDA 13.2, Python 3.12)
# Set the power mode first (see README), then run:  bash install.sh
set -e

VENV="$HOME/yolo-env"

echo "== System =="
cat /etc/nv_tegra_release
python3 --version

echo "== Virtual environment =="
sudo apt update
sudo apt install -y python3-pip python3-venv
python3 -m venv "$VENV" --system-site-packages
source "$VENV/bin/activate"
pip install -U pip

echo "== PyTorch (cu132, includes Orin sm_87) =="
pip install --pre torch torchvision --extra-index-url https://download.pytorch.org/whl/cu132
python - <<'EOF'
import torch
cap = torch.cuda.get_device_capability(0)
x = torch.rand(1000, 1000, device="cuda")
print("torch", torch.__version__, "| capability", cap, "| matmul", (x @ x).sum().item())
if cap != (8, 7):
    print("WARNING: expected (8, 7) for Jetson Orin")
EOF

echo "== Ultralytics =="
pip install ultralytics
pip install -U scipy
python -c "import torch, ultralytics; print('torch', torch.__version__, '| ultralytics', ultralytics.__version__)"

echo "== First detection =="
yolo predict model=yolo26n.pt source='https://ultralytics.com/images/bus.jpg' device=0

echo "== TensorRT FP16 export (takes several minutes) =="
python -c "import tensorrt; print('TensorRT', tensorrt.__version__)"
yolo export model=yolo26n.pt format=engine half=True device=0

echo "Done. Activate later with: source $VENV/bin/activate"
echo "Then compare speeds with: python benchmark.py"
