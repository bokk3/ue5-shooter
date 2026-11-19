import psutil

for p in psutil.process_iter(['pid', 'name', 'cmdline', 'cpu_percent']):
    try:
        cmd = ' '.join(p.info['cmdline'] or [])
        if any(k in cmd.lower() for k in ['unreal', 'noble', 'build_temp', 'dotnet', 'uht', 'cl.exe']):
            print(f"{p.info['pid']} {p.info['name']}: {cmd[:120]}")
    except Exception:
        pass
