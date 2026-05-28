class Counter is
  var n = 0
  this() is end
  method inc() is
    this.n := this.n.Plus(1)
    return
  end
  method get() : Integer is
    return this.n
  end
end

class Reporter is
  this() is end
  method report(c: Counter) is
    IO().WriteLine("count = ".Concatenate(c.get().ToString()))
    return
  end
end

class Main is
  this() is
    var c : Counter()
    var r : Reporter()
    c.inc()
    c.inc()
    c.inc()
    r.report(c)
  end
end
