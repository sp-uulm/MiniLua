function func1()
  func2()
end

function func2()
  func3()
end

function func3()
  error("some error")
end

func1()
