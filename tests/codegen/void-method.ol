class Main is
  method greet(name: String) is
    IO().WriteLine("Hello, ".Concatenate(name))
    return
  end

  method greetTwice(name: String) is
    this.greet(name)
    this.greet(name)
  end

  this() is
    this.greetTwice("world")
  end
end
