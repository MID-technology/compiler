class Main is
  this() is
    var a : Integer(5)
    var b : Integer(10)
    IO().WriteLine(a.Less(b).ToString())
    IO().WriteLine(a.LessEqual(a).ToString())
    IO().WriteLine(a.Greater(b).ToString())
    IO().WriteLine(b.GreaterEqual(a).ToString())
    IO().WriteLine(a.Equal(a).ToString())
    IO().WriteLine(a.Equal(b).ToString())
  end
end
