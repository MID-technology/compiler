class Main is
  method sumTo(n: Integer) : Integer is
    var i = 1
    var acc = 0
    while i.LessEqual(n) loop
      acc := acc.Plus(i)
      i := i.Plus(1)
    end
    return acc
  end

  this() is
    IO().WriteLine(this.sumTo(10).ToString())
  end
end
