HelplessHuman = {}

function HelplessHuman:main()
	self:readInput()
--	print(self.message)
 	print(tostring(self:parseMessage()))
end

function HelplessHuman:readInput()
	self.message = ""
	while(true) do
		local s = io.read()
		if s == "" then 
			break
		end
		self.message = self.message .. s
	end
end

function HelplessHuman:parseMessage()
	local list = {}
	local text = false
	local countString = 0

	for i=1, string.len(self.message) do
		local c = self.message:sub(i,i)
		if c == '"' then
			text = not text
			countString = countString + 1
		elseif text then
			--continue
		elseif c == '(' or c == '{' or c == '[' or c == '<' then
			table.insert(list, c)
		else
			if #list > 0 then
				if (c == ')' and list[#list] == '(') or 
				   (c == '}' and list[#list] == '{') or
				   (c == ']' and list[#list] == '[') or
				   (c == '>' and list[#list] == '<') then
					table.remove(list, #list)
				elseif c == ')' or c == ']' or c == '}' or c == '>' then
					return false
				end
			elseif c == ')' or c == ']' or c == '}' or c == '>' then
				return false
			end
		end
	end
	return #list == 0 and countString % 2 == 0
end

function HelplessHuman:tableSize(table)
	local size = 0
	for _ in pairs(table) do
		size = size + 1
	end
	return size
end

HelplessHuman:main()
