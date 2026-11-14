#!/usr/bin/env python3
"""Stage two independent browser builds for one project Pages deployment."""
from pathlib import Path
import argparse
import shutil
import tempfile
from html.parser import HTMLParser

class EditionMarker(HTMLParser):
    edition=None
    def handle_starttag(self,tag,attributes):
        attributes=dict(attributes)
        if tag=='meta' and attributes.get('name')=='wandering-edition':
            self.edition=attributes.get('content')

def edition(build):
    marker=EditionMarker();marker.feed((build/'wandering.html').read_text())
    return marker.edition

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--web',type=Path,required=True)
    parser.add_argument('--webxr',type=Path,required=True)
    parser.add_argument('--output',type=Path,default=Path('build-pages/wandering'))
    args=parser.parse_args()
    web,xr,output=args.web.resolve(),args.webxr.resolve(),args.output.resolve()
    root=Path(__file__).resolve().parents[1]
    if (web==xr or output==root or output in root.parents or
        any(output==p or output in p.parents or p in output.parents for p in (web,xr))):
        parser.error('Use distinct build directories and an output directory outside the sources.')
    required=['wandering.html','wandering.js','wandering.wasm','assets/scripts/content.lua',
              'assets/scripts/home.lua','assets/textures/atlas.rgba','assets/shaders/retro.vert',
              'assets/shaders/retro.frag','assets/fonts/hearth.fnt','assets/audio/chime.wav']
    for build in (web,xr):
        for name in required:
            if not (build/name).is_file():
                parser.error(f'Missing build output: {build/name}')
    if edition(web)!='web':
        parser.error('The standard build must use -DWC_WEBXR=OFF.')
    if edition(xr)!='webxr':
        parser.error('The WebXR build must use -DWC_WEBXR=ON.')
    output.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='wandering-pages-',dir=output.parent) as temp:
        staged=Path(temp)/'site';staged.mkdir()
        for build,destination in ((web,staged),(xr,staged/'webxr')):
            destination.mkdir(exist_ok=True)
            shutil.copy2(build/'wandering.html',destination/'index.html')
            for name in ['wandering.js','wandering.wasm']:
                shutil.copy2(build/name,destination/name)
            shutil.copytree(build/'assets',destination/'assets')
            for name in ['LICENSE','README.md']:
                shutil.copy2(root/name,destination/name)
            for name in ['docs','openspec']:
                shutil.copytree(root/name,destination/name)
        (staged/'.nojekyll').touch()
        if output.exists():shutil.rmtree(output)
        shutil.move(str(staged),str(output))
    print(f'Standard web: {output}/index.html')
    print(f'WebXR:        {output}/webxr/index.html')

if __name__=='__main__':main()
