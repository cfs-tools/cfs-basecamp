print "Hello, World.  Lua engine is starting."

function TestFunc()
    print "This is TestFunc()"

    cmd = EdsDB.GetInterface("CFE_ES/Application/CMD")
    testobj = EdsDB.NewMessage(cmd, "NoopCMD")

    print("obj=" .. EdsDB.ToHexString(testobj))

    CFE.SendMsg(testobj)
end

print "Completed Lua engine startup."
