class Main is
  method add(a: Integer, b: Integer) : Integer => a.Plus(b)
  method square(x: Integer) : Integer => x.Mult(x)

  this() is
    IO().WriteLine(this.add(3, 4).ToString())
    IO().WriteLine(this.square(5).ToString())
  end
end
