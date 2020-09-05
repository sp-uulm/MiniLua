FragmentedFurniture = {}
FragmentedFurniture.friends = 0
FragmentedFurniture.chairs = 0

function FragmentedFurniture:readInput()
	self.friends = io.read("*n")
	self.chairs = io.read("*n")

	for i=1, self.friends do
		self.chairs = self.chairs + io.read("*n")
	end
end

function FragmentedFurniture:chooseOutput()
	print(tostring(math.min(self.friends, self.chairs - 1)))
end

FragmentedFurniture:readInput()
FragmentedFurniture:chooseOutput()
