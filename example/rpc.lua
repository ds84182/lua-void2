local void = require "void"
local thread = require "llthreads2"
local socket = require "socket"

local struct = dofile("rpc_struct.lua")

local request, requestID = void.queue.create(16)
local reply, replyID = void.queue.create(16)

print(requestID, replyID)

local function loadfile_thread(fname, ...)
	local fh = io.open(fname, "r")
	local code = fh:read("*a")
	fh:close()
	return thread.new(code, ...)
end

local rpcthread = loadfile_thread("rpc_thread.lua", requestID, replyID)
rpcthread:start(false, false)

socket.sleep(1)

local start = os.clock()

local nextRequestID = 0

local function sendKill()
	local requestID = nextRequestID
	nextRequestID = requestID+1

	void.queue.enqueue(request,
        void.struct.write(struct.method_kill, {
            method = 0, id = requestID
        })
    )
    print("Sent stop request")

	return requestID
end

local function sendAddRequest(a, b)
	local requestID = nextRequestID
	nextRequestID = requestID+1

	void.queue.enqueue(request, 
        void.struct.write(struct.method_add, {
            method = 1,
            id = requestID,
            a = a,
            b = b
        })    
    )
    print("Sent add request")

	return requestID
end

local function waitResponse(id)
	while true do
		local buffer = void.queue.await(reply, 1)
		print(buffer)
		if buffer and void.buffer.type(buffer) ~= "invalid" then
			-- TODO: Compare ID
			return buffer
		end
	end
end

local id = sendAddRequest(5.5, 10.3)
local response = void.struct.read(struct.method_add, waitResponse(id))

print(response.a)

sendKill()
rpcthread:join()

print((os.clock()-start)*1000)
