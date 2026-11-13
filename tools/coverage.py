"""Measure authored authoritative C line coverage; fail below 95%, never infer it."""
import argparse, gzip, json, pathlib, subprocess, sys
p=argparse.ArgumentParser()
p.add_argument('build',type=pathlib.Path)
p.add_argument('--gcov',default='gcov')
p.add_argument('--minimum',type=float,default=95.0)
a=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1]
out=root/'artifacts'/'coverage';out.mkdir(parents=True,exist_ok=True)
gcov=str(pathlib.Path(a.gcov).resolve()) if pathlib.Path(a.gcov).exists() else a.gcov
notes=list(a.build.resolve().glob('CMakeFiles/wc_core.dir/src/*.gcno'))
if len(notes)!=3:
    sys.exit('Expected coverage notes for core.c, save.c and content.c. Build with WC_COVERAGE=ON.')
files=[]
for note in notes:
    subprocess.run([gcov,'-j','-b','-c',str(note)],cwd=out,check=True,capture_output=True)
    with gzip.open(out/(note.stem+'.gcov.json.gz'),'rt') as f: data=json.load(f)
    for entry in data['files']:
        if pathlib.Path(entry['file']).name not in ('core.c','save.c','content.c'):continue
        lines=entry['lines'];branches=[b for line in lines for b in line.get('branches',[])]
        missing=[line['line_number'] for line in lines if line['count']==0]
        files.append(dict(file=entry['file'],lines=len(lines),hit=len(lines)-len(missing),
                          branches=len(branches),branches_hit=sum(b['count']>0 for b in branches),missing=missing))
total=sum(f['lines'] for f in files);hit=sum(f['hit'] for f in files)
branch_total=sum(f['branches'] for f in files);branch_hit=sum(f['branches_hit'] for f in files)
report=dict(scope='Authored authoritative C: core.c, save.c, content.c. Excludes SDL/GL adapters and dependencies.',
            total_lines=total,covered_lines=hit,line_percent=100*hit/total,
            total_branches=branch_total,covered_branches=branch_hit,branch_percent=100*branch_hit/branch_total,files=files)
(out/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
if report['line_percent']<a.minimum:sys.exit(f'Coverage below {a.minimum}%')
