import os
import subprocess

def UseVld():
    while True:
        reply = str(input("Use VLD? [Y/n]: ")).lower().strip()
        if (reply == "y" or reply == ""):
            return True
        else:
            print(f"Invalid argument {reply}.")

os.chdir("../")

useVld = UseVld()

if (useVld):
    subprocess.call(["vendor/bin/premake5.exe", "vs2022", "--use-vld"])
else:
    subprocess.call(["vendor/bin/premake5.exe", "vs2022"])