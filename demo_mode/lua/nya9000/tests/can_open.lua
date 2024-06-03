Sequence("CanOpen", function()
    Test("MetaFunction", function()
        Context.map.ibs.board1.od = nil
        Context.map.ibs.board2.od = nil

        Log.i("Board 1")
        Utils.print(Context.map.ibs.board1, 2)
        Context.map.ibs.board1:foo()

        Log.i("Board 2")
        Utils.print(Context.map.ibs.board2, 2)
        Context.map.ibs.board2:foo()
    end)
end)
