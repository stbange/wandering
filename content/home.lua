-- MIT. Editable content, loaded beside the executable. Coordinates in metres.
-- kind: 0 decoration, 1 helm, 2 bed, 3 pantry, 4 workshop, 5 planter, 6 gate, 7 map.
return {
  tuning = { speed=5, turbo=5, turn=0.38, walk=3.3, sprint=5.5,
             jump=5.5, gravity=15, eye=1.62 },
  objects = {
    { id=1, kind=1, slot=0, min={-0.7,0.6,-6.3}, max={0.7,2.0,-5.6}, color={0.56,0.33,0.15}, solid=true },
    { id=2, kind=7, slot=0, min={2.6,0.6,-6.5}, max={4.6,1.5,-5.1}, color={0.26,0.44,0.39}, solid=true },
    { id=3, kind=2, slot=0, min={-5.4,0,-0.8}, max={-3.6,0.65,2}, color={0.39,0.43,0.60}, solid=true },
    { id=4, kind=3, slot=0, min={3.9,0,3.7}, max={5.4,1.0,5.0}, color={0.48,0.30,0.18}, solid=true },
    { id=5, kind=4, slot=0, min={3.9,0,-1.4}, max={5.4,1.0,1.4}, color={0.50,0.34,0.23}, solid=true },
    { id=6, kind=5, slot=0, min={-5.4,0,3.8}, max={-3.7,0.8,5.1}, color={0.37,0.25,0.16}, solid=true },
    { id=7, kind=5, slot=1, min={-5.4,0,5.5}, max={-3.7,0.8,6.8}, color={0.37,0.25,0.16}, solid=true },
    { id=8, kind=6, slot=0, min={1.35,0.8,7.25}, max={1.8,1.9,7.7}, color={0.75,0.52,0.20}, solid=false }
  }
}
