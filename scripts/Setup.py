import os
import subprocess
import argparse

def UseVld():
    while True:
        reply = str(input("Use VLD to detect memory leaks? [y/N]: ")).lower().strip()
        if (reply == "y"):
            return True
        elif (reply == "n" or reply == ""):
            return False
        else:
            print(f"Invalid argument {reply}.")

def UseAftermath():
    while True:
        reply = str(input("Include Nsight Aftermath for GPU crash detection? (only works on an NVIDIA GPU!!!) [y/N]: ")).lower().strip()
        if (reply=="y"):
            return True
        elif (reply == "n" or reply == ""):
            return False
        else:
            print(f"Invalid argument {reply}.")

os.chdir("../")

parser = argparse.ArgumentParser(prog="Hyper Setup", description="Setup script for Hyper")
parser.add_argument("--minimal", help="Quickly setup a project with minimal features (no VLD, no Nsight Aftermat, ...)", action="store_true")
args = parser.parse_args()

options = []
useVld = False
useAftermath = False

if (not args.minimal):
    useVld = UseVld()
    useAftermath = UseAftermath()


if (useVld):
    options.append("--use-vld")
if (useAftermath):
    options.append("--use-aftermath")

if (len(options)):
    subprocess.call(["vendor/bin/premake5.exe", "vs2022", *options])
else:
    subprocess.call(["vendor/bin/premake5.exe", "vs2022"])
