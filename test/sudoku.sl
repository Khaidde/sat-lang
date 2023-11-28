
property Num {
  _1
  _2
  _3
  _4
  _5
  _6
  _7
  _8
  _9
}

grid board[9][9][9]

function is_sat {
  if !board[0][2][Num._5] { return false }

  if !board[6][1][Num._9] { return false }

  if !board[3][8][Num._2] { return false }

  for m in 9 {
    for n in 9 {
      for i in 9 {
        if board[0][m][n] && board[i][m][n] {
          return false
        }
        if board[m][0][n] && board[m][i][n] {
          return false
        }
        if board[m][n][0] && board[m][n][i] {
          return false
        }
      }
    }
  }

  return true
}

