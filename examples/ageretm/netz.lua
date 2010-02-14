function TimeToSlot(time)
  local tslots = time * yats.SlotBitRate / 53 / 8
  if tslots >= 1 then
    return tslots
  else
    return 1
  end
end

function SlotToTime(slot)
  return slot * 53 * 6 / yats.SlotBitRate
end

function iseven(x)
  if math.mod(x,2) == 0 then
    event = 1
  else
    even = 0
  end
end

