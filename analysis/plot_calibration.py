from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd

ROOT = Path(__file__).resolve().parent.parent
DATA, DOCS = ROOT / "data", ROOT / "docs"
pts = pd.read_csv(DATA / "calibration_points.csv")

rows = []
for _, r in pts.iterrows():
    mv = pd.read_csv(DATA / r["file"])["soil_mv"].tail(60)
    rows.append((r["moisture_pct"], mv.mean(), mv.std()))
cal = pd.DataFrame(rows, columns=["moisture_pct", "mv_mean", "mv_std"])
print(cal.round(1))

plt.errorbar(cal["mv_mean"], cal["moisture_pct"], xerr=cal["mv_std"], fmt="o-", capsize=3)
plt.xlabel("Sensor output (mV)")
plt.ylabel("Kadar air gravimetrik (%)")
plt.title("Kalibrasi sensor kapasitif (interpolasi antar titik)")
plt.grid(alpha=0.3)
DOCS.mkdir(exist_ok=True)
plt.savefig(DOCS / "calibration.png", dpi=150, bbox_inches="tight")