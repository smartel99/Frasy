local future = MakeThread(function(v)
    delay(1.0)
    return v + 1
end, 1)

while future:wait_for(0.5) == future.timeout do
    print("waiting for future...")
end

print("Value: " .. future:get())


