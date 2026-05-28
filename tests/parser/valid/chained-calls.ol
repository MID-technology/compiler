class Main is
  this() is
    var x = 1.Plus(2).Mult(3).Minus(4)
    var s = "a".Concatenate("b").Concatenate("c")
    IO().WriteLine(s.Concatenate(x.ToString()))
  end
end
