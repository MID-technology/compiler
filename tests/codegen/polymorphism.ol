class Animal is
  this() is end
  method Sound() : String is
    return "generic"
  end
end

class Dog extends Animal is
  this() is end
  method Sound() : String is
    return "woof"
  end
end

class Cat extends Animal is
  this() is end
  method Sound() : String is
    return "meow"
  end
end

class Main is
  this() is
    var a : Animal()
    a := Dog()
    IO().WriteLine(a.Sound())
    a := Cat()
    IO().WriteLine(a.Sound())
  end
end
