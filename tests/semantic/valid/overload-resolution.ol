class Main is
  method pick(a: Integer) : String is
    return "int"
  end
  method pick(a: Integer, b: Integer) : String is
    return "two ints"
  end
  method pick(a: String) : String is
    return "string"
  end

  this() is
    IO().WriteLine(this.pick(1))
    IO().WriteLine(this.pick(1, 2))
    IO().WriteLine(this.pick("hi"))
  end
end
