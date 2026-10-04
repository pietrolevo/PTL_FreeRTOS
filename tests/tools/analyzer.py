from collections import defaultdict

class JobState:
    def __init__(self):
        self.start = None
        self.end = None
        self.miss = False
        self.killed = False
        self.skipped = False
        self.overrun = False
        self.catch_up = False

def analyze_events(events):
    registry = defaultdict(lambda: defaultdict(JobState))
    for ev in events:
        j = registry[ev["task"]][ev["job"]]
        act = ev["action"]
        
        if act == "START":
            j.start = ev["time"]
        elif act in ["COMPLETE", "KILL", "ABORTED"]:
            j.end = ev["time"]
        if act == "DEADLINE_MISS":
            j.miss = True
        elif act in ["KILL", "ABORTED"]:
            j.killed = True
        elif act == "SKIP":
            j.skipped = True
        elif act == "OVERRUN":
            j.overrun = True
        elif act == "CATCH_UP":
            j.catch_up = True
            
    return registry
