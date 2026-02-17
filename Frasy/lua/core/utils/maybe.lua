--- Checks if a value is nil or meets a predicate.
--- @param v any? Value to check
--- @param f function Predicate that takes v and returns a boolean
return function(v, f) return type(v) == "nil" or f(v) end
