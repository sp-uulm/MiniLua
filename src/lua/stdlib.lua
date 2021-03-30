function assert(predicate, message)
    if not predicate then
        if message ~= nil then
            error(message)
        else
            error("assertion failed!")
        end
    end
end

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
