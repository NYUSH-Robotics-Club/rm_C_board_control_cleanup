# Python acts as the recipe interpreter; Windows does not need sh or Git Bash.
set shell := ["python3", "tools/firmware.py", "--just"]
set windows-shell := ["py", "-3", "tools/firmware.py", "--just"]

default:
    help

configure robot mode="Debug" run="no":
    configure {{robot}} --mode {{mode}} --run-after {{run}} --allow-single yes

doctor:
    doctor

build robot="":
    build {{robot}}

flash robot="":
    flash {{robot}}

flash-plan robot="":
    flash-plan {{robot}}
