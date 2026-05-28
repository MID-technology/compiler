class Shape is
  this() is end
  method area() : Integer is
    return 0
  end
end

class Square extends Shape is
  var side = 0
  this(s: Integer) is
    this.side := s
  end
  method area() : Integer is
    return this.side.Mult(this.side)
  end
end

class Main is
  this() is
    var sq : Square(4)
    IO().WriteLine(sq.area().ToString())
  end
end
