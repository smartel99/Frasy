Sequence("R8L", function()
    Test("Download", function()
        local ib = Context.map.ibs.r8l
        ib:Download(ib.od["Write Digital Output"], 0xFF)
        Utils.sleep_for(100)
        ib:Download(ib.od["Write Digital Output"], 0x00)
    end)
end)
