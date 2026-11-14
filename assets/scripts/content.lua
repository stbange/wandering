-- MIT. Stable zero-based item IDs are a save contract. Cost arrays are one-based.
local function cost(t) local c={} for id,n in pairs(t) do c[id+1]=n end return c end
return {
  speed=5, turbo=5, turn=0.38,
  equipment={{damage=12,power=1,armor=0},{damage=28,power=5,armor=.1}},
  room_cost=cost{[5]=6,[6]=2,[1]=4},
  salvage=cost{[5]=2,[6]=2,[12]=1},
  reserve=cost{[5]=6,[6]=2,[12]=4},
  items={
    {name='Timber',buy=2,sell=1},{name='Stone',buy=2,sell=1},
    {name='Fiber',buy=2,sell=1},{name='Sunroot seed',buy=3,sell=1},
    {name='Mint seed',buy=3,sell=1},{name='Planks',buy=3,sell=2},
    {name='Twine',buy=4,sell=2},{name='Sunroot',buy=3,sell=2},
    {name='Meadow mint',buy=3,sell=2},{name='Compost',buy=4,sell=2},
    {name='Field satchel',buy=16,sell=8},{name='Mechanism wedge',buy=5,sell=2},
    {name='Foundry alloy',buy=8,sell=4},{name='Trail stew',buy=9,sell=5},
    {name='Packed cabinet',buy=8,sell=4},{name='Packed compact',buy=6,sell=3},
  },
  recipes={
    {name='Saw 2 planks',cost=cost{[0]=2},output=5,quantity=2},
    {name='Spin 2 twine',cost=cost{[2]=3},output=6,quantity=2},
    {name='Mix 2 compost',cost=cost{[7]=1,[8]=1,[1]=1},output=9,quantity=2},
    {name='Sew field satchel (+16)',cost=cost{[5]=2,[6]=2},output=10},
    {name='Shape mechanism wedge',cost=cost{[0]=2,[1]=1},output=11,unlock=1},
    {name='Cook trail stew',cost=cost{[7]=2,[8]=1},output=13,unlock=2},
  },
  crops={{name='Sunroot',seconds=45,yield=3},{name='Meadow mint',seconds=60,yield=3}},
}
