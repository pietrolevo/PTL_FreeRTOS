import sys
import os


sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from parser import LogParser
from analyzer import analyze_events

os.set_blocking(sys.stdout.fileno(), True)

def main():
    test_name = sys.argv[1] if len(sys.argv) > 1 else "ptl_test"
    verbose = "-verbose" in sys.argv

    parser = LogParser()
    events, units, task_stats = [], [], []
    busy_stats, result = None, None
    jitter_violation = False
    raw_lines = []

    for line in sys.stdin:
        raw_lines.append(line)
        if verbose:
            print(line, end="")
        if "[FATAL] Jitter Violation" in line:
            jitter_violation = True

        res = parser.parse(line)
        if not res:
            continue
        if res["type"] == "event":
            events.append(res)
        elif res["type"] == "task_stats":
            task_stats.append(res)
        elif res["type"] == "busy_stats":
            busy_stats = res
        elif res["type"] == "result":
            result = res
        elif res["type"] == "unit":
            units.append(res)

    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    logs_dir = os.path.join(base_dir, "logs")
    os.makedirs(logs_dir, exist_ok=True)

    with open(os.path.join(logs_dir, f"{test_name}.trace"), "w") as f:
        f.writelines(raw_lines)

    # --- Verdict ---
    if units:
        overall = all(u["passed"] for u in units)
    elif result is not None:
        overall = result["passed"]
    else:
        overall = False

    if jitter_violation:
        overall = False

    # NFR checks
    nfr_pass = True

    for ts in task_stats:
        if ts["max_jitter"] > 1:
            nfr_pass = False

    overall = overall and nfr_pass

    overhead: int = 0
    if busy_stats is not None:
        if busy_stats["total"] > 0:
            overhead = (busy_stats["busy"] / busy_stats["total"] * 100)
        else:
            overhead = 0

    # Report
    rpt = [f"=== PTL TEST REPORT: {test_name} ===\n\n"]

    if units:
        failed = [u["name"] for u in units if not u["passed"]]
        rpt.append(f"[UNITS] {len(units) - len(failed)}/{len(units)} passed\n")
        for name in failed:
            rpt.append(f"  [FAIL] {name}\n")

    if result:
        rpt.append(
            f"[RESULT] {result['name']}: {'PASSED' if result['passed'] else 'FAILED'}\n"
        )

    for ts in task_stats:
        rpt.append(
            f"[TASK] {ts['task']}: jobs={ts['jobs']} misses={ts['misses']} "
            f"overruns={ts['overruns']} maxJitter={ts['max_jitter']} "
            f"({'OK' if ts['max_jitter'] <= 1 else 'JITTER FAIL'})\n"
        )

    if busy_stats:
        rpt.append(
            f"[NFR] CPU overhead: {overhead:.2f}% ({'PASS' if overhead <= 10 else 'FAIL'})\n"
        )

    rpt.append(
        f"[NFR] Jitter Violation hook: {'FAIL' if jitter_violation else 'PASS'}\n"
    )
    rpt.append(f"\n=== OVERALL: {'PASSED' if overall else 'FAILED'} ===\n")

    with open(os.path.join(logs_dir, f"{test_name}_report.rpt"), "w") as f:
        f.writelines(rpt)

    # Console summary
    print(
        f"[{test_name}] {'PASSED' if overall else 'FAILED'}  "
        f"(log: tests/logs/{test_name}.trace, report: tests/logs/{test_name}_report.rpt)"
    )

    # Gantt chart
    try:
        from plot import save_chart

        if events:
            report_jobs = analyze_events(events)
            save_chart(test_name, report_jobs)
    except ModuleNotFoundError:
        print("[WARNING] Install plotly")

    sys.exit(0 if overall else 1)


if __name__ == "__main__":
    main()
