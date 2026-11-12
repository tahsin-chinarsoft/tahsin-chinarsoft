# Ollama + Open WebUI on Jetson Orin Nano Super (8 GB, JetPack 7.2)

Video: https://youtu.be/jTg0tJ2IExo · Short: https://youtube.com/shorts/AJQlhw63cIs

Local LLMs on an 8 GB Jetson: install / update Ollama, benchmark 4 models with the same question, add Open WebUI, and check what really fits in memory.

## Results

Ollama 0.40.16 · JetPack 7.2 · MAXN SUPER · 256 tokens · temperature 0 · 1 warm-up + 3 timed runs

| Model | Size | Generation speed | `ollama ps` PROCESSOR |
|---|---|---|---|
| qwen3.5:2b | 2.7 GB | **22.2 tok/s** | 100% GPU |
| llama3.2:3b | 2.0 GB | **20.9 tok/s** | 100% GPU |
| gemma3:4b | 3.4 GB | **9.3 tok/s** | 33%/67% CPU/GPU |
| qwen2.5:7b | 4.7 GB | – | killed by the OOM killer |

With Open WebUI running (container ≈ 924 MiB), qwen3.5:2b dropped to 37%/63% CPU/GPU and only 141 MiB was left available.

## 1. Check the starting point

```bash
ollama --version; systemctl is-active ollama; free -h; sudo nvpmodel -q | head -1
```

## 2. Install / update Ollama

```bash
curl -fsSL https://ollama.com/install.sh | sh
```

`WARNING: Unsupported JetPack version detected. GPU may not be supported` comes from the install script's version list. It does not mean the GPU is off. Check it:

```bash
ollama run llama3.2:3b "Explain OEE in one sentence." --verbose
ollama ps        # PROCESSOR column -> 100% GPU
```

In `--verbose`: `load duration` = disk → memory (first run only), `prompt eval rate` = reading the prompt, **`eval rate` = writing the answer** (the number that matters).

## 3. Pull the models and benchmark

```bash
ollama pull qwen3.5:2b
ollama pull gemma3:4b
ollama pull qwen2.5:7b
python3 bench_ollama.py      # writes ollama_bench.csv
```

Why the 7B failed:

```bash
journalctl -u ollama --since "10 min ago" | grep -i oom
# ollama.service: A process of this unit has been killed by the OOM killer.
```

CPU and GPU share the same 8 GB (7.3 GiB visible, ~5 GB free for a model, no swap). Model file + KV cache + GPU buffers must fit.

## 4. Open WebUI (Docker)

```bash
sudo docker run -d --network host --restart always \
  -e OLLAMA_BASE_URL=http://127.0.0.1:11434 -e PORT=8080 \
  -v open-webui:/app/backend/data --name open-webui \
  ghcr.io/open-webui/open-webui:main
```

Open `http://<jetson-ip>:8080` and create the admin account. Select a 2–3 B model (the default may be the 7B → "Server disconnected").

Memory check with everything running:

```bash
ollama ps; free -h; sudo docker stats --no-stream --format "{{.Name}}  {{.MemUsage}}"
```

## Tips

- 2–3 B models stay on the GPU (~20 tok/s). 4 B is the limit, 7 B does not fit without extra tricks.
- Run Open WebUI on another computer to leave the Jetson's memory for the model.
- New topic → new chat (the whole conversation is re-sent; context is 4096 tokens here).
- Small models make confident mistakes. Verify answers and code before using them.

## Stop everything / restore

```bash
sudo docker stop open-webui && sudo docker update --restart=no open-webui
sudo systemctl disable --now ollama
# back on:  sudo systemctl enable --now ollama ; sudo docker start open-webui
```
