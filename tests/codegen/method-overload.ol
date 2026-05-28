class Main is
  method add(a: Integer) : Integer is
    return a.Plus(1)
  end
  method add(a: Integer, b: Integer) : Integer is
    return a.Plus(b)
  end
  method add(a: Integer, b: Integer, c: Integer) : Integer is
    return a.Plus(b).Plus(c)
  end

  this() is
    IO().WriteLine(this.add(10).ToString())
    IO().WriteLine(this.add(10, 20).ToString())
    IO().WriteLine(this.add(10, 20, 30).ToString())
  end
end
