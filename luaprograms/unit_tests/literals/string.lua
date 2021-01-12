-- Excerpt from https://www.lua.org/manual/5.1/manual.html#2.8
--
-- Literal strings can be delimited by matching single or double quotes, and can contain the following C-like escape sequences: '\a' (bell), '\b' (backspace), '\f' (form feed), '\n' (newline), '\r' (carriage return), '\t' (horizontal tab), '\v' (vertical tab), '\\' (backslash), '\"' (quotation mark [double quote]), and '\'' (apostrophe [single quote]). Moreover, a backslash followed by a real newline results in a newline in the string. A character in a string can also be specified by its numerical value using the escape sequence \ddd, where ddd is a sequence of up to three decimal digits. (Note that if a numerical escape is to be followed by a digit, it must be expressed using exactly three digits.) Strings in Lua can contain any 8-bit value, including embedded zeros, which can be specified as '\0'.
--
-- Literal strings can also be defined using a long format enclosed by long brackets. We define an opening long bracket of level n as an opening square bracket followed by n equal signs followed by another opening square bracket. So, an opening long bracket of level 0 is written as [[, an opening long bracket of level 1 is written as [=[, and so on. A closing long bracket is defined similarly; for instance, a closing long bracket of level 4 is written as ]====]. A long string starts with an opening long bracket of any level and ends at the first closing long bracket of the same level. Literals in this bracketed form can run for several lines, do not interpret any escape sequences, and ignore long brackets of any other level. They can contain anything except a closing bracket of the proper level.
--
-- For convenience, when the opening long bracket is immediately followed by a newline, the newline is not included in the string. As an example, in a system using ASCII (in which 'a' is coded as 97, newline is coded as 10, and '1' is coded as 49), the five literal strings below denote the same string:
--
-- The a* variables are also taken from that url:

a1 = 'alo\n123"'
a2 = "alo\n123\""
a3 = '\97lo\10\04923"'
a4 = [[alo
123"]]
a5 = [==[
alo
123"]==]

assert("hello" == 'hello')
assert("\t\n123\0hi" == '\t\n123\0hi')
-- assert("\t\n123\0\x23" == '\t\n123\0#')
assert("\"'" == '"\'')
assert("\070" == "F")

-- multiline
-- assert("hello\nworld", "hello\
-- world")

assert([[hi
world]] == "hi\nworld")

assert([=[hi[[
world]]!]=] == "hi[[\nworld]]!")

assert([===[hi[==[\n
world\"]==]!]===] == "hi[==[\\n\nworld\\\"]==]!")

-- ignores newline if it is the first character
assert([[
hello world!]] == "hello world!")

assert(a1 == a2)
assert(a2 == a3)
assert(a3 == a4)
assert(a4 == a5)
