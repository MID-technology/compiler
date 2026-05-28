class Main is
  method countDivisible(n: Integer, d: Integer) : Integer is
    var i = 1
    var cnt = 0
    while i.LessEqual(n) loop
      if i.Rem(d).Equal(0) then
        if i.Greater(1) then
          cnt := cnt.Plus(1)
        end
      end
      i := i.Plus(1)
    end
    return cnt
  end

  this() is
    IO().WriteLine(this.countDivisible(20, 3).ToString())
  end
end
