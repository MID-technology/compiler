class Main is
  this() is
    var a : Integer(20)
    var b : Integer(7)
    IO().WriteLine(a.Plus(b).ToString())
    IO().WriteLine(a.Minus(b).ToString())
    IO().WriteLine(a.Mult(b).ToString())
    IO().WriteLine(a.Div(b).ToString())
    IO().WriteLine(a.Rem(b).ToString())
    IO().WriteLine(a.UnaryMinus().ToString())
  end
end
