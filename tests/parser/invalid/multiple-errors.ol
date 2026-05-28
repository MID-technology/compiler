class First
  this() is
    IO().WriteLine("missing is above")
  end
end

class Second is
  var
  method m() is
    if true
      IO().WriteLine("missing then")
    end
  end
end
