class Main is
  method check(n: Integer) : String is
    if n.Greater(0).And(n.Less(10)) then
      return "in range"
    end
    return "out of range"
  end
  this() is
    var i = 0
    while i.Less(3).Or(false) loop
      IO().WriteLine(this.check(i))
      i := i.Plus(1)
    end
  end
end
