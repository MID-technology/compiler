class Main is
  this() is
    var x : Real(3.5)
    var n : Integer(2)
    IO().WriteLine(x.Plus(n).ToString())
    IO().WriteLine(x.Minus(1.5).ToString())
    IO().WriteLine(x.Mult(n).ToString())
    IO().WriteLine(x.Div(n).ToString())
    IO().WriteLine(x.UnaryMinus().ToString())
    IO().WriteLine(n.toReal().Plus(0.5).ToString())
    IO().WriteLine(x.toInteger().ToString())
  end
end
