--[[
Lotta Laps

Lotta is trainging her endurance with interval training. For that she
has picked a specific route (you don't know the length of) through the
local park. This morning she ran the route again and again, each time
with a different but constant speed which she tracked. Hoping to 
encourage her in her efforts, you want to create some francy diagrams
with the statistics. Therefore you calculate different values, e.g. 
Lotta's highest speed, the average speed per round, the total average
speed over all rounds and so on. For some reason, you have some 
problems with calculating the average speed, so you write a program to
help you with that.

Input:
One line with an integer n (1 <= n <= 1000), the numer of times Lotta
has run the route.
One line with n integers v1, ..., vn (1 <= vi <= 10^5 for all i), the
speeds in km/h Lotta run at.

Output:
Output the total average speed over all rounds in km/h Lotta ran at.

Samples:
Sample1:
	Input:
	2
	6 12

	Output:
	8

Sample2:
	Input:
	3
	1 983 2

	Output (rounded):
	1.9986445
--]]

LottaLaps = {}
LottaLaps.sum = 0.0

function LottaLaps:readInput()
	n = io.read("*n")
	for i=1, n do
		self.sum = self.sum + 1/io.read("*n")
	end
end

function LottaLaps:round2(num, numDecimalPlaces)
	return tonumber(string.format("%." .. 
		(numDecimalPlaces or 0) .. "f", num))
end

function LottaLaps:main()
	self:readInput()
	print(tostring(n/self.sum))
end

LottaLaps:main()
