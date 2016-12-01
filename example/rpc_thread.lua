local void = require "void"

local struct = dofile("rpc_struct.lua")

local request, reply = ...
print(request, reply)
request, reply = void.queue.get(request), void.queue.get(reply)
print(request, reply)

while true do
	local buffer = void.queue.await(request, -1)
	print(buffer)

	if buffer and void.buffer.type(buffer) ~= "invalid" then
        print("Got")
        local request = void.struct.read(struct.method, buffer)

		print(request.method, request.id)

		if request.method == 0 then
			break
		elseif request.method == 1 then
            request = void.struct.read(struct.method_add, buffer)
			local c = request.a+request.b
			print(request.a, request.b, c)
            request.a = c
			void.queue.enqueue(reply, void.struct.write(struct.method_add, buffer, request))
		end
	end
end
