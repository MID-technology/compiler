class A is
  this() is end
  method baseValue() : Integer is
    return 1
  end
end

class B extends A is
  this() is end
  method extra() : Integer is
    return this.baseValue().Plus(10)
  end
end

class Main is
  this() is
    var b : B()
    IO().WriteLine(b.baseValue().ToString())
    IO().WriteLine(b.extra().ToString())
  end
end
