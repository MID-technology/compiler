class Main is
  method classify(n: Integer) : String is
    if n.Greater(0) then
      return "positive"
    else
      if n.Less(0) then
        return "negative"
      else
        return "zero"
      end
    end
  end

  this() is
    IO().WriteLine(this.classify(5))
    IO().WriteLine(this.classify(0))
    IO().WriteLine(this.classify(7.UnaryMinus()))
  end
end
