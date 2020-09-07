--[[
Fragmented Furniture:

Karl wants to invite his friends over for a party. However, he doesn't
have a lot of chairs, and he wants to make sure that nobody will have
to stand, including himself. So he tells each friend to bring as many
chairs as they can. With that help, what's the maximum number of 
friends he can invite to his home?

Input:
One line with two integers n and k (1 <= n, k <= 100), 
n = number of friends, k = number of avaiable Chairs that Karl has

Output:
The maximum number of friends that Karl can invite over, assuming each
invitee comes and brings as many chairs as they can.

Samples:
Sample1:
	Input:
	6 2
	1 0 0 2 1 0

	Output:
	5

Sample2:
	Input:
	3 7
	100 100 100

	Output:
	3

Sample3:
	Input:
	1 1
	0

	Output:
	0
]]--

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
