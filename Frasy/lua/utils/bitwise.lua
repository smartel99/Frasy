local bitwise = {}

function bitwise.Inject(index, value, cache)
    cache = cache & ~(1 << index)
    cache = cache | ((value & 1) << index)
    return cache
end

function bitwise.Extract(index, value) return ((value >> index) & 1) end

return bitwise
