class Main is
  method foo(a: Integer, b: Integer) : Integer is
    return a.Plus(b)
  end
  this() is
    IO().WriteLine(this.foo(1).ToString())
  end
end
