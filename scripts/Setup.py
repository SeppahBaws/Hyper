import os
import subprocess

def UseVld():
    while True:
        reply = str(input("Use VLD? [y/N]: ")).lower().strip()
        if (reply == "y"):
            return True
        elif (reply == "n" or reply == ""):
            return False
        else:
            print(f"Invalid argument {reply}.")

os.chdir("../")

useVld = UseVld()

if (useVld):
    subprocess.call(["vendor/bin/premake5.exe", "vs2022", "--use-vld"])
else:
    subprocess.call(["vendor/bin/premake5.exe", "vs2022"])