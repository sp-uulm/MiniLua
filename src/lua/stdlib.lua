
function assert(predicate, message)
    if not predicate then
        if message != nil then
            error(message)
        else
            error("assertion failed!")
        end
    end
end
