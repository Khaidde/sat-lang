
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

grid house_color[5]

grid house_pet[5]

function is_sat {
  x = true

  if house_color[Color.Blue] {
    x = house_color[Color.Red]
  }
  if house_pet[Pet.Dog] {
    return x
  }

  return true
}
