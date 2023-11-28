
property Color {
  Red
  Blue
  Green
  Yellow
  White
}

property Pet {
  Fish
  Dog
  Bird
  Cat
  Horse
}

object House {
  Color
  Pet
}

House neighborhood[16][16]

function is_sat {
  x = false

  if neighborhood[4][1][0][Green] {
    return neighborhood[1][1][Pet][Cat]
  }
  if neighborhood[4][1][0][Green] {
    return neighborhood[1][1][Pet][Cat]
  }
  if neighborhood[4][1][0][Green] {
    return neighborhood[1][1][Pet][Cat]
  }
  if neighborhood[4][1][0][Green] {
    return neighborhood[1][1][Pet][Cat]
  }
  return true
}
