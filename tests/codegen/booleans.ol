class Main is
  this() is
    var t : Boolean(true)
    var f : Boolean(false)
    IO().WriteLine(t.And(f).ToString())
    IO().WriteLine(t.Or(f).ToString())
    IO().WriteLine(t.Xor(f).ToString())
    IO().WriteLine(t.Not().ToString())
    IO().WriteLine(t.toInteger().ToString())
  end
end
