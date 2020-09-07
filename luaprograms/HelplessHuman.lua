--[[
When writing text, people sometimes rely on braces to inject 
additional details to certain statements. Due to human defectiveness,
braces (especially closing ones are prone to be missing. A different 
race - the compilers - heavily relies on braces to be balanced and 
well nested. To prevent misapprehensions and ensure proper interaction
between humans and compilers, all humans' texts need to be verified 
prior to passing them on to the compilers.
The pairs of characters (), [], {} and <> are used as control symbols
in source code and each define a control block. An opening brace 
(i.e., one of ([{<) marks the start of a control block, the 
corresponding closing brace (i.e., the matching brace )]}>) marks the
end of the control block. Control blocks can encapsulate other control
blocks, but no two control blocks may intersect each other. A text may
also contain string literals, which are character sequences delimited
by a pair of quotation marks ". Quotation marks cancel out control 
characters, which means that after the first quotation mark no 
no character is interpreted as control character until the next 
quotation mark. Write a source code check that tests if all string 
literals and control blocks are valid and closed.

Input:
One or more lines (separated by \n) made up of whitespaces, numbers,
alphabetic characters, and all other printable ASCII characters. More
precisely, the set of allowed characters is the ASCII range 0x20-0x7e,
as well as 0x09 (tab) and 0x0a (newline).

Output:
Output correct if all control blocks and strings are valid and closed
or incorrect if not.

Samples:
Sample1:
	Input:
	{[<"this is a > string">]
	this is not a string}()

	Output:
	correct

Sample2:
	Input:
	{[<"this is a > string"]
	this is not a string}()

	Output:
	incorrect
--]]

HelplessHuman = {}

function HelplessHuman:main()
	self:readInput()
--	print(self.message)
 	if self:parseMessage() then
		print("correct")
	else
		print("incorrect")
	end
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
		elseif c == '(' or c == '{' or c == '[' or c == '<' 
			then
			table.insert(list, c)
		else
			if #list > 0 then
				if (c == ')' and list[#list] == '(') or 
				   (c == '}' and list[#list] == '{') or
				   (c == ']' and list[#list] == '[') or
				   (c == '>' and list[#list] == '<') 
				   then
					table.remove(list, #list)
				elseif c == ')' or c == ']' or 
					c == '}' or c == '>' then
					return false
				end
			elseif c == ')' or c == ']' or c == '}' or 
				c == '>' then
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
