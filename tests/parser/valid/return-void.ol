class Main is
  method nothing() is
    return
  end

  method nothingNoReturn() is
    var x = 1
  end

  this() is
    this.nothing()
    this.nothingNoReturn()
  end
end
