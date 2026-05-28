class Main is
  method isEven(n: Integer) : Boolean
  method isOdd(n: Integer) : Boolean

  method isEven(n: Integer) : Boolean is
    if n.Equal(0) then
      return true
    end
    return this.isOdd(n.Minus(1))
  end

  method isOdd(n: Integer) : Boolean is
    if n.Equal(0) then
      return false
    end
    return this.isEven(n.Minus(1))
  end

  this() is
    IO().WriteLine(this.isEven(4).ToString())
    IO().WriteLine(this.isOdd(7).ToString())
  end
end
