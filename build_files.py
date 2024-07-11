import subprocess
import os,sys
import contextlib
try:
    subprocess.run(['kicad-cli'],capture_output=True)
except FileNotFoundError:
    #if kicad-cli not found, run inside of docker
    cwd=os.getcwd()
    cmd=["docker", "run", "--rm", f"-v{cwd}:{cwd}",f"-w{cwd}","-it","kicad/kicad:8.0.2","python3"]
    cmd+=sys.argv
    #print(cmd)
    proc = subprocess.run(cmd)
    sys.exit(proc.returncode)

@contextlib.contextmanager
def pushd(new_dir):
    previous_dir = os.getcwd()
    os.chdir(new_dir)
    try:
        yield
    finally:
        os.chdir(previous_dir)
        
def sh_run(cmd,capture_output=False):
    print(f"run:{cmd}")
    return subprocess.run(cmd,shell=True,capture_output=capture_output,text=True)


def pcbway_gen(project_name,project_rev):
    sh_run(f"sed -i 's/Revision:.*\"/Revision:{project_rev}\"/' {project_name}.kicad_pcb")
    sh_run(f"kicad-cli pcb export drill {project_name}.kicad_pcb  -o manufacture/")
    sh_run(f"kicad-cli pcb export gerbers  {project_name}.kicad_pcb  --board-plot-params -o manufacture")
    with pushd("manufacture"):
        sh_run(f"zip {project_name}-gbr.zip *.drl *.gbr *.gbrjob")
        #sh_run(f"rm  *.drl *.gbr *.pos")
    sh_run(f"kicad-cli pcb export pos --side front --use-drill-file-origin {project_name}.kicad_pcb -o manufacture/front.pos")
    sh_run(f"kicad-cli pcb export pos --side back --use-drill-file-origin {project_name}.kicad_pcb -o manufacture/back.pos")
    sh_run("kicad-cli sch export bom -o manufacture/bom.csv --group-by Value "+
           " --fields '${ITEM_NUMBER},Reference,${QUANTITY},MANUFACTURER,MPN,Value,Footprint,${DNP}'"+
           " --labels 'Item #,Designator,Qty,Manufacturer,Mfg Part#,Value,Footprint,dnp'" +
           f" {project_name}.kicad_sch")
    sh_run(f"kicad-cli sch export pdf -D PROJECT_REV={project_rev} -o manufacture/schematic.pdf {project_name}.kicad_sch")
def jlcpcb(project_name,project_rev):
    sh_run(f"sed -i 's/Revision:.*\"/Revision:{project_rev}\"/' {project_name}.kicad_pcb")
    sh_run(f"kicad-cli pcb export drill {project_name}.kicad_pcb  -o jlcpcb_manufacture/")
    sh_run(f"kicad-cli pcb export gerbers  {project_name}.kicad_pcb  --board-plot-params -o jlcpcb_manufacture")
    with pushd("jlcpcb_manufacture"):
        sh_run(f"zip {project_name}-gbr.zip *.drl *.gbr *.gbrjob")
        #sh_run(f"rm  *.drl *.gbr *.pos")
    sh_run(f"kicad-cli pcb export pos --format csv --units mm --use-drill-file-origin --side both {project_name}.kicad_pcb -o jlcpcb_manufacture/position.csv")
    sh_run("kicad-cli sch export bom -o jlcpcb_manufacture/bom.csv --group-by Value "+
           " --fields 'Reference,Footprint,${QUANTITY},Value,Footprint,LCSC Part #'"+
           " --labels 'Designator,Footprint,Qty,Value,LCSC Part #'" +
           f" {project_name}.kicad_sch")
    sh_run(f"kicad-cli sch export pdf -D PROJECT_REV={project_rev} -o jlcpcb_manufacture/schematic.pdf {project_name}.kicad_sch")
    
if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("project_name")
    args= parser.parse_args()
    githash=sh_run("git describe --tags --always --dirty",capture_output=True).stdout.strip()
    pcbway_gen(args.project_name,githash)
    #jlcpcb(args.project_name,githash)
