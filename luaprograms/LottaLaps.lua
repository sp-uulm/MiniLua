LottaLaps = {}
LottaLaps.sum = 0.0

function LottaLaps:readInput()
	n = io.read("*n")
	for i=1, n do
		self.sum = self.sum + 1/io.read("*n")
	end
end

function LottaLaps:round2(num, numDecimalPlaces)
	return tonumber(string.format("%." .. (numDecimalPlaces or 0) .. "f", num))
end

function LottaLaps:main()
	self:readInput()
	print(tostring(n/self.sum))
end

LottaLaps:main()
