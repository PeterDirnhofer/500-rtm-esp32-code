Import('env')
import os
import shutil
import time

# This script runs as a PlatformIO extra script. It runs after the
# `buildprog` target and copies the produced firmware into `artifacts/`.

def after_build(source, target, env):
    proj_dir = env['PROJECT_DIR']
    build_dir = os.path.join(proj_dir, '.pio', 'build', env['PIOENV'])
    src = os.path.join(build_dir, 'firmware.bin')
    if not os.path.isfile(src):
        # fallback: try common names
        candidates = ['firmware.bin', 'firmware.elf', 'firmware.bin']
        found = None
        for c in candidates:
            p = os.path.join(build_dir, c)
            if os.path.isfile(p):
                found = p
                break
        if not found:
            print('move_firmware: no firmware found in', build_dir)
            return
        src = found

    artifacts_dir = os.path.join(proj_dir, 'artifacts')
    os.makedirs(artifacts_dir, exist_ok=True)

    # Write only the global constant artifact (overwrite atomically)
    const_path = os.path.join(artifacts_dir, 'firmware.bin')
    try:
        tmp_path = const_path + '.tmp'
        shutil.copy2(src, tmp_path)
        os.replace(tmp_path, const_path)
        print(f'move_firmware: updated constant artifact {const_path}')
    except Exception as e:
        print('move_firmware: failed to write constant artifact:', e)

# Register to run after build
env.AddPostAction('buildprog', after_build)
