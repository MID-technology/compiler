class A is
  this() is end
  method who() : String is
    return "A"
  end
  method shared() : String is
    return "from A"
  end
end

class B extends A is
  this() is end
  method who() : String is
    return "B"
  end
end

class C extends B is
  this() is end
  method who() : String is
    return "C"
  end
end

class Main is
  this() is
    var x : A()
    x := C()
    IO().WriteLine(x.who())
    IO().WriteLine(x.shared())
  end
end
