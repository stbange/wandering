#!/usr/bin/env python3
"""Package existing verified builds; never embeds assets into the executable."""
from pathlib import Path
import argparse, shutil, tarfile, zipfile
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--native',default='build-native');p.add_argument('--web',default='build-web');args=p.parse_args()
out=root/'dist';out.mkdir(exist_ok=True)
for kind,build in [('linux-x86_64',root/args.native),('web',root/args.web)]:
    target=out/f'wandering-{kind}';target.mkdir(exist_ok=True)
    names=['wandering.html','wandering.js','wandering.wasm'] if kind=='web' else ['wandering']
    for name in names:shutil.copy2(build/name,target/name)
    shutil.copytree(root/'assets',target/'assets',dirs_exist_ok=True)
    for name in ['LICENSE','README.md']:shutil.copy2(root/name,target/name)
    shutil.copytree(root/'docs',target/'docs',dirs_exist_ok=True)
    shutil.copytree(root/'openspec',target/'openspec',dirs_exist_ok=True)
    if kind=='web':
        with zipfile.ZipFile(out/'wandering-web.zip','w',zipfile.ZIP_DEFLATED) as z:
            for file in target.rglob('*'):
                if file.is_file():z.write(file,file.relative_to(target))
    else:
        with tarfile.open(out/'wandering-linux-x86_64.tar.gz','w:gz') as tar:tar.add(target,arcname=target.name)
    print(target)
