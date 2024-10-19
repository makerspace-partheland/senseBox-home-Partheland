Import("env")
import os

project_name = env["PIOENV"]
# change firmware name  
env.Replace(PROGNAME="firmware_%s_%s" %  (project_name, env.GetProjectOption("custom_prog_version")))