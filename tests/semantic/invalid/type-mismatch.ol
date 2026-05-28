class Main is
    method foo(x: Integer) : Integer is
        return x.Plus(1)
    end
    this() is
        var result : this.foo("hello")
        var result2 : this.foo(3.12)
    end
end