Sequence("CanOpen", function()
    Test("Upload", function()
        local ib = Context.map.ibs.daq
        ib:Upload(ib.od.Identity)
    end)
    Test("Download", function()
        local ib = Context.map.ibs.daq
        local tx = 0
        local rx = 0
        ib:Download(ib.od["Communication cycle period"], tx)
        rx = ib:Upload(ib.od["Communication cycle period"])
        Expect(tx):ToBeEqual(rx)
        tx = 12
        ib:Download(ib.od["Communication cycle period"], tx)
        rx = ib:Upload(ib.od["Communication cycle period"])
        Expect(tx):ToBeEqual(rx)
    end)
end)
