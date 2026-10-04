import os
import plotly.graph_objects as go
import plotly.io as pio

def save_chart(test_name, data_jobs, auto_open=False):


    def _compute_grid_step(max_tick, target_lines=25):
        if max_tick <= 0:
            return 20
        raw_step = max_tick / target_lines
        magnitude = 10 ** (len(str(int(raw_step))) - 1)
        for multiplier in (1, 2, 5, 10):
            step = multiplier * magnitude
            if step >= raw_step:
                return step
        return magnitude * 10

    fig = go.Figure()
    max_tick = 0
    
    kill_x = []
    kill_y = []
    kill_hover = []
    
    for t_name, jobs in data_jobs.items():
        for j_id, job in jobs.items():
            if job.start is not None: max_tick = max(max_tick, job.start)
            if job.end is not None: max_tick = max(max_tick, job.end)
            if job.start is None: continue

            duration = job.end - job.start if (job.end and job.end > job.start) else 0.5
            
            # Color encoding: Green = RUNNING, Orange = SKIP, Red = KILL
            if job.skipped:
                color = 'rgb(255, 159, 64)'  # Orange
                status = "SKIPPED"
            elif job.killed:
                color = 'rgb(219, 68, 85)'   # Red
                status = "KILLED"
                kill_x.append(job.end)
                kill_y.append(t_name)
                kill_hover.append(f"<b>Task:</b> {t_name}<br><b>Job ID:</b> {j_id}<br><b>KILLED at tick:</b> {job.end}")
            else:
                color = 'rgb(40, 167, 69)'   # Green
                status = "RUNNING"
                if job.catch_up:
                    color = 'rgb(40, 167, 150)'  # different Green
                    status = "RUNNING (CATCH_UP / MISS)"
            
            fig.add_trace(go.Bar(
                x=[duration], y=[t_name], base=job.start, orientation='h',
                marker=dict(color=color, line=dict(color='black', width=1)),
                name=f"{t_name} - J{j_id}", text=f"J{j_id}", textposition='inside',
                hovertemplate=(
                    f"<b>Task:</b> {t_name}<br>"
                    f"<b>Job ID:</b> {j_id}<br>"
                    f"<b>Start Tick:</b> {job.start}<br>"
                    f"<b>End Tick:</b> {job.end if job.end is not None else 'N/A'}<br>"
                    f"<b>Duration:</b> {duration} ticks<br>"
                    f"<b>PTL Status:</b> {status}<extra></extra>"
                )
            ))

    if kill_x:
        fig.add_trace(go.Scatter(
            x=kill_x,
            y=kill_y,
            mode='markers',
            marker=dict(
                symbol='x',
                size=14,
                color='rgb(180, 0, 0)',
                line=dict(width=2)
            ),
            text=kill_hover,
            hoverinfo='text',
            showlegend=False
        ))

    grid_step = _compute_grid_step(max_tick)
    gridlines = [dict(type='line', x0=t, y0=-0.5, x1=t, y1=len(data_jobs)-0.5,
                    line=dict(color='rgba(0,0,0,0.15)', width=1, dash='dash')) 
                for t in range(0, max_tick + grid_step, grid_step) if t > 0]


    fig.update_layout(
        title=f"PTL Scheduling Gantt Chart - Test: {test_name} (Aquamarine = CATCH_UP, Green=RUNNING, Orange=SKIP, Red=KILL)",
        xaxis_title="Ticks (Time, 100 tick = 1 ms)", yaxis_title="Tasks", barmode='stack', showlegend=False,
        shapes=gridlines, height=300 + (len(data_jobs) * 50)
    )
    fig.update_yaxes(autorange="reversed")
    
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    output_html = os.path.join(base_dir, "logs", f"{test_name}_gantt.html")
    
    pio.write_html(fig, file=output_html, auto_open=auto_open)

    print(f"[PLOT] chart generated and saved to: tests/logs/{test_name}_gantt.html")
