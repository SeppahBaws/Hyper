from pathlib import Path
import os
import shutil

def GetConfirmation():
    while True:
        reply = str(input("Are you sure? [y/N]: ")).lower().strip()
        if (reply == "y"):
            return True
        else:
            return False

extensions = [
    "sln",
    "vcxproj",
    "vcxproj.filters",
    "vcxproj.user"
]
dirs = [
    "../bin",
    "../bin-int"
]

paths = []

for extension in extensions:
    for path in Path('../').rglob(f"*.{extension}"):
        paths.append(path)

for path in paths:
    print(f"removing {path}")
    os.remove(path)
for directory in dirs:
    print(f"removing {directory}")
    shutil.rmtree(directory)

print("Cleanup complete!")
