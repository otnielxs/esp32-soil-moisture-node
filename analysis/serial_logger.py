import argparse
import csv
import time
from datetime import datetime

import serial  # pip install pyserial

HEADER = ["pc_time", "ms", "soil_mv", "soil_pct", "temp_c", "rh", "light_raw", "state"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--seconds", type=int, default=0, help="0 = sampai Ctrl+C")
    ap.add_argument("--baud", type=int, default=115200)
    a = ap.parse_args()

    t_end = time.time() + a.seconds if a.seconds > 0 else None
    with serial.Serial(a.port, a.baud, timeout=2) as ser, open(a.out, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(HEADER)
        try:
            while t_end is None or time.time() < t_end:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                parts = line.split(",")
                if len(parts) != 7 or parts[0] == "ms":
                    continue
                w.writerow([datetime.now().isoformat(timespec="seconds")] + parts)
                f.flush()
        except KeyboardInterrupt:
            pass


if __name__ == "__main__":
    main()