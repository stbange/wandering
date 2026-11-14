-- MIT Lua code; original geometry/palette art CC BY 4.0. Measurements in metres.
local home={}
local function box(x,y,z,w,h,d,m) home[#home+1]={x=x,y=y,z=z,w=w,h=h,d=d,m=m} end
-- Workshop, raised command dais, oak helm and brass wheel housing.
box(-4,.45,-1,2.2,.9,1.2,2);box(-4,.94,-1,2.5,.12,1.4,7)
box(-4,1.2,-1,.5,.4,.45,3);box(-4.7,1.05,-1,.35,.1,.7,5)
box(0,.16,-5.7,5,.32,3.2,2);box(0,.7,-5.8,.35,1.1,.35,3)
box(0,1.25,-5.85,1.3,.12,.55,5)
box(2.1,.8,-5.3,1.5,1.2,1,2);box(2.1,1.42,-5.3,1.6,.06,1.1,8)
-- Bed, pantry, rugs and shelves.
box(4,.25,4,2.5,.5,3,2);box(4,.57,4,2.25,.25,2.8,9)
box(4,.76,3.05,1.9,.16,.6,8);box(4,.74,4.4,2.28,.12,1.5,11)
box(-4,.5,4,2.4,1,1.3,2);box(-4,1.05,4,2.5,.12,1.4,5)
box(-4.8,.6,4,.14,.2,1.42,3);box(-3.2,.6,4,.14,.2,1.42,3)
box(0,.012,1,4,.024,4.8,11)
for i=1,3 do box(-5.65,i*.65,1.4,.6,.12,2,2) end
-- Timber ceiling beams, chimney and warm brass lanterns.
for z=-6,6,3 do box(0,3.45,z,12,.35,.25,2) end
for x=-1,1,2 do box(x*5.5,2.1,6,.18,.6,.18,5);box(x*5.5,2.2,6,.3,.28,.3,10) end
box(-4.8,3.8,-5,.7,3,.7,1)
return {
 title='WANDERING CASTLE', home=home,
 palette={
  {r=.49,g=.60,b=.32}, -- 0 meadow
  {r=.47,g=.50,b=.51}, -- 1 stone
  {r=.31,g=.20,b=.15}, -- 2 oak
  {r=.19,g=.24,b=.27}, -- 3 iron
  {r=.29,g=.45,b=.29}, -- 4 leaf
  {r=.76,g=.47,b=.24}, -- 5 copper
  {r=.48,g=.31,b=.23}, -- 6 soil
  {r=.68,g=.47,b=.28}, -- 7 pine
  {r=.90,g=.84,b=.65}, -- 8 parchment
  {r=.84,g=.77,b=.56}, -- 9 linen
  {r=1,g=.77,b=.32},   -- 10 lantern
  {r=.38,g=.35,b=.39}, -- 11 rug
  {r=.39,g=.63,b=.67}, -- 12 glass
  {r=.77,g=.32,b=.26}, -- 13 rust
  {r=.68,g=.73,b=.43}, -- 14 sunroot
  {r=.52,g=.73,b=.57}, -- 15 mint
 }
}
