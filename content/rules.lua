-- MIT. Scripted home activities. C validates and commits each returned command.
-- Panels: bed=2, pantry=3, workshop=4, planter=5. Indices are zero-based.
-- This script has no filesystem, network or OS APIs.
return {
  on_action = function(panel, index, slot)
    if panel == 2 then
      if index == 0 then return {command="rest"} end
      if index == 1 then return {command="save"} end
    elseif panel == 3 then
      if index == 0 then return {command="select-item"} end
      if index == 1 then return {command="deposit"} end
      if index == 2 then return {command="withdraw"} end
    elseif panel == 4 then
      return {command="craft", id=index}
    elseif panel == 5 then
      return {command="garden", id=index}
    end
    return {command="unavailable"}
  end
}
