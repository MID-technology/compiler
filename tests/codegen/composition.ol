class Point is
  var x = 0
  var y = 0
  this(px: Integer, py: Integer) is
    this.x := px
    this.y := py
  end
  method describe() : String is
    return "(".Concatenate(this.x.ToString())
      .Concatenate(", ")
      .Concatenate(this.y.ToString())
      .Concatenate(")")
  end
end

class Segment is
  var a : Point(0, 0)
  var b : Point(0, 0)
  this(p1: Point, p2: Point) is
    this.a := p1
    this.b := p2
  end
  method describe() : String is
    return this.a.describe().Concatenate(" -> ").Concatenate(this.b.describe())
  end
end

class Main is
  this() is
    var p1 : Point(1, 2)
    var p2 : Point(3, 4)
    var s : Segment(p1, p2)
    IO().WriteLine(s.describe())
  end
end
