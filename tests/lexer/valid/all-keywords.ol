// Exercises every keyword and punctuator the lexer must recognize.
class A is end

class B extends A is
  var flag = true
  var other = false

  method short(x: Integer) : Integer => x.Plus(1)

  method body(n: Integer) : Integer is
    var i = 0
    while i.Less(n) loop
      if i.Equal(0) then
        i := i.Plus(1)
      else
        return i
      end
    end
    return i
  end

  this() is end
end
