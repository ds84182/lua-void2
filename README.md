# lua-void2
Binary serialization and message passing for Lua 5.1+. Probably doesn't build on Windows.

Uses thread_compat from llthreads2.

This library does not spawn threads! Use llthreads2 to spawn threads!

## Usage Example

See ref.txt for some barebones API docs.

```lua
  local queue = void.queue.create(10)
	local rpcstruct = void.struct.create {
		packed = true, -- Do not insert any padding bytes (For cross arch support)
		{"method", "u8"},
		{"id", "u32le"},
		{"size", "u32le"},
		{"data", "buffer", size = "size"}
	}

	local nextRPCID = 0
	local function sendRPC(method, data)
		local structdata = {
			method = method,
			id = nextRPCID,
			data = data -- Since buffer size is attached to the size field, size gets automatically filled out
		}

		local length = void.struct.length(rpcstruct, structdata)
		local buffer = void.buffer.fromStruct(rpcstruct, structdata)

		void.queue.enqueue(queue, buffer, true) -- We want to suspend execution if the buffer is full
		nextRPCID = nextRPCID+1

		return nextRPCID-1
	end

	local readfilestruct = void.struct.create {
		packed = true, -- Do not insert any padding bytes (For cross arch support)
		{"filenameSize", "u32le"},
		{"filename", "buffer", size = "filenameSize"},
		{"modeSize", "u32le"},
		{"mode", "buffer", size = "modeSize"}
	}

	local function readFileAsync(filename, mode, callback)
		local id = sendRPC(RPC_METHOD_READ_FILE, void.buffer.fromStruct(readfilestruct, {filename=filename,mode=mode}))
		addCallback(id, callback)
	end

	-- Thread code:
	-- Best worker thread ever~
	function workerthread(queueID)
		local queue = void.queue.fromID(queueID)
		local rpcstruct = void.struct.create {
			packed = true, -- Do not insert any padding bytes (For cross arch support)
			{"method", "u8"},
			{"id", "u32le"},
			{"size", "u32le"},
			{"data", "buffer", size = "size"}
		}

		local readfilestruct = void.struct.create {
			packed = true, -- Do not insert any padding bytes (For cross arch support)
			{"filenameSize", "u32le"},
			{"filename", "buffer", size = "filenameSize"},
			{"modeSize", "u32le"},
			{"mode", "buffer", size = "modeSize"}
		}

		while true do
			local rpcbuffer = void.queue.await(queue)
			local rpc = void.struct.read(rpcbuffer, rpcstruct)

			if rpc.method == RPC_METHOD_READ_FILE then
				local arguments = void.struct.read(rpc.data, readfilestruct)
				local file = io.open(arguments.filename, arguments.mode)
				local length = file:seek("end")
				file:seek("begin")
				local data = void.buffer.create(length)
				void.buffer.readFile(file, length)
				file:close()

				-- More stuff here to send back the data buffer...
			end
		end
	end
```
