#!/usr/bin/env python3
"""Ollama speed test on Jetson: 1 warm-up + 3 timed runs per model."""
import csv, json, subprocess, time, urllib.request

MODELS = ["qwen3.5:2b", "llama3.2:3b", "gemma3:4b", "qwen2.5:7b"]
PROMPT = "Explain what OEE means in manufacturing and give one example."
RUNS = 3
URL = "http://127.0.0.1:11434/api/generate"

def gen(model):
    body = json.dumps({"model": model, "prompt": PROMPT, "stream": False,
                       "options": {"num_predict": 256, "temperature": 0}}).encode()
    req = urllib.request.Request(URL, body, {"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=900) as r:
        return json.load(r)

rows = []
for m in MODELS:
    try:
        gen(m)  # warm-up (loads the model)
    except Exception as e:
        print(f"{m:14s} SKIPPED ({e})"); continue
    ps = subprocess.run(["ollama", "ps"], capture_output=True, text=True).stdout.strip().splitlines()
    proc = next((l for l in ps if l.startswith(m.split(':')[0])), "")
    rates, ttft = [], []
    for _ in range(RUNS):
        d = gen(m)
        rates.append(d["eval_count"] / (d["eval_duration"] / 1e9))
        ttft.append((d.get("load_duration", 0) + d["prompt_eval_duration"]) / 1e9)
    avg = sum(rates) / len(rates)
    rows.append([m, f"{avg:.1f}", f"{min(rates):.1f}", f"{max(rates):.1f}", f"{sum(ttft)/len(ttft):.2f}", proc])
    print(f"{m:14s} {avg:6.1f} tok/s  (min {min(rates):.1f}, max {max(rates):.1f})  first token ~{sum(ttft)/len(ttft):.2f}s")
    print(f"               {proc}")
    subprocess.run(["ollama", "stop", m], capture_output=True)

with open("ollama_bench.csv", "w", newline="") as f:
    w = csv.writer(f); w.writerow(["model", "avg_tok_s", "min", "max", "first_token_s", "ollama_ps"]); w.writerows(rows)
print("saved ollama_bench.csv")
