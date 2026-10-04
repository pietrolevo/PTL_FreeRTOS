import re


class LogParser:
    def __init__(self):
        self.ev_re = re.compile(r"^\[\s*(\d+)\]\s+([A-Z_]+)\s+(\w+)\s+(\d+)")
        self.task_re = re.compile(
            r"^\[STATS\]\s+task=(\S+)\s+jobs=(\d+)\s+misses=(\d+)\s+overruns=(\d+)\s+maxJitter=(\d+)"
        )
        self.busy_re = re.compile(r"^\[STATS\]\s+busy_ticks=(\d+)\s+total_ticks=(\d+)")
        self.result_re = re.compile(r"^\[RESULT\]\s+(.+):\s+(PASSED|FAILED)")
        self.pass_re = re.compile(r"^\[PASS\]\s+(.+)")
        self.fail_re = re.compile(r"^\[FAIL\]\s+(.+)")

    def parse(self, line):
        line = line.strip()

        m = self.ev_re.match(line)
        if m:
            return {
                "type": "event",
                "time": int(m.group(1)),
                "action": m.group(2),
                "task": m.group(3),
                "job": int(m.group(4)),
            }

        m = self.task_re.match(line)
        if m:
            return {
                "type": "task_stats",
                "task": m.group(1),
                "jobs": int(m.group(2)),
                "misses": int(m.group(3)),
                "overruns": int(m.group(4)),
                "max_jitter": int(m.group(5)),
            }

        m = self.busy_re.match(line)
        if m:
            return {
                "type": "busy_stats",
                "busy": int(m.group(1)),
                "total": int(m.group(2)),
            }

        m = self.result_re.match(line)
        if m:
            return {
                "type": "result",
                "name": m.group(1),
                "passed": m.group(2) == "PASSED",
            }

        m = self.pass_re.match(line)
        if m:
            return {"type": "unit", "name": m.group(1), "passed": True}

        m = self.fail_re.match(line)
        if m:
            return {"type": "unit", "name": m.group(1), "passed": False}

        return None
