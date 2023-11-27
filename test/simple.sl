
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
  x = !true && false || !true

  if false || !true {
    y = neighborhood[4][1][Color][Red]
  }
  y = neighborhood[4][1][0][Green]
  return true
}
