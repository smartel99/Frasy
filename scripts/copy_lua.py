import os
import shutil
import pathlib

root_dir = os.path.abspath(pathlib.Path(__file__).parent.parent.resolve())
bin_dir = os.path.join(root_dir, "bin")
lua_dir = os.path.join(root_dir, "Frasy", "lua")

for config in os.listdir(bin_dir):
    config_dir = os.path.join(bin_dir, config)
    for project in os.listdir(config_dir):
        project_dir = os.path.join(config_dir, project)
        for file in os.listdir(project_dir):
            if ".exe" in file:
                target_dir = os.path.join(project_dir, "lua")
                shutil.rmtree(target_dir, ignore_errors=True)
                shutil.copytree(src=lua_dir, dst=target_dir)
                pass
