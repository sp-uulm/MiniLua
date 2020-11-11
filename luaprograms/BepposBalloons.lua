--[[
Beppos Balloons

Your local community has a big summer festival every year where all 
the people of the district come together to enjoy the weather, eat 
hearty food, drink some beverages and watch different show acts. This
year, at one point the clown Beppo is performing tricks for the 
audience. After the big show he stays around and plays different fun
games with all who want to participate. At the moment Beppo has n 
shells in front of hin. He puts a small coin under one of them and 
starts swapping shells around with each swap exchaning the positions 
of exactly two shells. After multiple swaps the next person in the 
line must guess under which shell the coin is abd if they are correct,
the clown makes a balloon animal for them.
You really want a balloon giraffe, so when it is your turn, you watch
the swaps closly. But, oh dear, Bepps really has quick hands. While 
you can often see the exact positions of the shells that get swaped, 
sometimes you miss one or even both. So in the end your best bet is to
just guessone of the possible positions the coin can be in.

Input:
- One line with 3 integers n,s and p(3<=n<=100, 1<=s<=10^5, 1<=p<=n),
the number of shells, the numberof swaps and the starting position of 
the coin.
- s lines, the ith of which contains two integers a and b(1<=a,b<=n or
a,b = -1) indicating
that the shell at position a was swapped with the shell at position b
in the ith swap. -1 means you missed the position of the corresponding
shell. Every swap is guaranteed to exchange the positions of exactly 
two shells.

Output:
Output all possible positions the coin can be in after performing all
swaps. You may output them in any order.

Samples:
Sample1:
	Input:
	10 3 8
	8 9
	4 3
	9 4

	Output:
	4

Sample2:
	Input:
	3 4 1
	3 -1
	1 2
	1 3
	-1 2

	Output:
	1 2 3

Sample3:
	Input:
	3 4 1
	3 2
	1 2
	1 3
	-1 2

	Output:
	1 3

Sample4:
	Input:
	5 3 4
	4 2
	-1 -1
	2 4

	Output:
	1 2 3 4 5
--]]

BepposBalloons = {}
BepposBalloons.shells = {}

function BepposBalloons:main()
	self:readInput()
	self:followSwaps()
	local places = self:findPossiblePlaces()
	for i,a in pairs(places) do
		io.write(tostring(a).." ")
	end
	io.write("\n")
end

function BepposBalloons:readInput()
	local n,s,p = io.read("*n","*n","*n")
	for i=1,n do
		table.insert(self.shells, false)
	end
	self.shells[p] = true
	self.swaps = s
end

function BepposBalloons:followSwaps()
	for i=1, self.swaps do
		local a,b = io.read("*n", "*n")
		if a ~= -1 and b ~= -1 then
			local tmp = self.shells[a]
			self.shells[a] = self.shells[b]
			self.shells[b] = tmp
		elseif a ~= -1 and b == -1 then
			local tmp = self.shells[a]
			local tmp2 = false
			for j = 1, #self.shells do
				if a == j then
					--continue
				else
					tmp2 = tmp2 or self.shells[j]
					if not self.shells[j] then
						self.shells[j] = tmp
					end
				end
			end
			self.shells[a] = tmp2
		elseif a == -1 and b ~= -1 then
			local tmp = self.shells[b]
			local tmp2 = false
			for j = 1, #self.shells do
				if b == j then
					--continue
				else
					tmp2 = tmp2 or self.shells[j]
					if not self.shells[j] then
						self.shells[j] = tmp
					end
				end
			end
			self.shells[b] = tmp2
		elseif a == -1 and a == -1 then
			for i = 1, #self.shells do
				self.shells[i] = true
			end
		end
	end
end

function BepposBalloons:findPossiblePlaces()
	local places = {}
	for i, b in pairs(self.shells) do
		if b then
			table.insert(places, i)
		end
	end
	return places
end

BepposBalloons:main()
