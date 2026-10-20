from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "data"
DOCS = ROOT / "docs"
points = pd.read_csv(DATA / "calibration_points.csv")

fig, ax = plt.subplots(figsize=(10, 6))
for _, r in points.iterrows():
    df = pd.read_csv(DATA / r["file"])
    t = (df["ms"] - df["ms"].iloc[0]) / 1000.0
    ax.plot(t, df["soil_mv"], label=f'{r["moisture_pct"]:.1f} %')
    blocks = df["soil_mv"].groupby((t // 30).astype(int)).mean().round(0).tolist()
    print(f'{r["file"]}: rata-rata per 30 s = {blocks} | min={df["soil_mv"].min()} max={df["soil_mv"].max()}')

ax.set_xlabel("Waktu sejak rekaman dimulai (s)")
ax.set_ylabel("Sensor output (mV)")
ax.legend(title="Kadar air")
ax.grid(alpha=0.3)
DOCS.mkdir(exist_ok=True)
fig.savefig(DOCS / "cal_traces.png", dpi=150, bbox_inches="tight")