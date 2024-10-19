Import("env")
import os
import shutil
import glob
import datetime

def afterBuild(source, target, env):
    print("After build")

#Move firmware to /build folder
    version = env.GetProjectOption("custom_prog_version")
    folder = env.GetProjectOption("custom_prog_folder")
    newpath = env["PROJECT_DIR"] + "/build/" + folder
    if not os.path.exists(newpath):
        os.makedirs(newpath)
	
    firmware_old = env["PROJECT_DIR"] + "/.pio/build/" + env["PIOENV"] + "/firmware_%s_%s.bin" %  (env["PIOENV"], version)
    firmware_name_new = newpath + "/firmware_%s_V%s_%s.bin" %  (env["PIOENV"], version, datetime.date.today())
    shutil.copy(firmware_old, firmware_name_new)
    print("Firmware moved to: " + env["PROJECT_DIR"] + "/build/firmware.bin")
    # Delete old firmware
    files = glob.glob(env["PROJECT_DIR"] + "/.pio/build/" + env["PIOENV"] + "/*.bin")
    #for f in files:
        #os.remove(f)
    print("Old firmware deleted")

project_name = env["PIOENV"]
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", afterBuild)
     
     #This script will rename the firmware.bin file to firmware_<project_name>_<custom_prog_version>.bin. 
     #Note:  The custom_prog_version is a custom build option that you can set in platformio.ini. 