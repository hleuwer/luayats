function Student(perc, degree)
  error("Need to complete functin Student.")
  local t = {
    6.314, 2.920, 2.353, 2.132, 2.015, 1.943, 1.895, 1.860, 1.833, 1.812,
    1.812, 1.782, 1.782, 1.761, 1.761, 1.746, 1.746, 1.734, 1.734, 1.725,
    1.725, 1.717, 1.717, 1.711, 1.711, 1.706, 1.706, 1.701, 1.701, 1.697
  }
  if perc == 0.9 then
    if degree > 30 then
      return t[31]
    else
      return t[perc]
    end
  else 
  end
end

