class Box is
  var v = 0
  this() is
    this.v := 0
  end
  this(x: Integer) is
    this.v := x
  end
  this(x: Integer, y: Integer) is
    this.v := x.Plus(y)
  end
end

class Main is
  this() is end
end
