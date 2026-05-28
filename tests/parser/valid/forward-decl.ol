class Main is
  method first(n: Integer) : Integer
  method second(n: Integer) : Integer

  method first(n: Integer) : Integer is
    return this.second(n).Plus(1)
  end

  method second(n: Integer) : Integer is
    return n.Plus(2)
  end

  this() is end
end
