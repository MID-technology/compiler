class Main is
  method run(n: Integer) : Integer is
    var i = 0
    while i.Less(n) loop
      if i.Equal(0) then
        i := i.Plus(1)
      else
        if i.Greater(5) then
          return i
        else
          i := i.Plus(2)
        end
      end
    end
    return i
  end
  this() is end
end
