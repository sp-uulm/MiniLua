function assert(predicate, message)
    if not predicate then
        if message ~= nil then
            error(message)
        else
            error("assertion failed!")
        end
    end
end

-- setup io
io.__default_out = io.stdout
io.__default_in = io.stdin

-- trivial io functions
function io.flush()
    return io.output():flush()
end

function io.read(...)
    return io.input():read(...)
end

function io.write(...)
    return io.output():write(...)
end

-- other io functions
function io.close(file)
    if file == nil then
        return io.output():close()
    elseif io.type(file) ~= nil then
        return file:close()
    else
        error("bad argument #1 to 'close' (file expected, got " .. type(file) .. ")")
    end
end

function io.input(file)
    if file == nil then
        return io.__default_in
    elseif io.type(file) ~= nil then
        io.__default_in = file
    elseif type(file) == "string" then
        handle, err = io.open(file, "r")
        if handle == nil then
            error(err)
        else
            io.__default_in = handle
        end
    else
        error("bad argument #1 to 'input' (string or file expected, got " .. type(file) .. ")")
    end
end

function io.output(file)
    if file == nil then
        return io.__default_out
    elseif io.type(file) ~= nil then
        io.__default_out = file
    elseif type(file) == "string" then
        local handle, err = io.open(file, "w")
        if handle == nil then
            error(err)
        else
            io.__default_out = handle
        end
    else
        error("bad argument #1 to 'output' (string or file expected, got " .. type(file) .. ")")
    end
end
