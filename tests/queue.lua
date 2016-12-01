local void = require "void"

local suite = {}

function suite.test_create()
	local queue = void.queue.create(10, "hi")
	lunatest.assert_userdata(queue)
	void.queue.destroy(queue)
end

function suite.test_get()
	local queue = void.queue.create(10, "test_get_queue")
	lunatest.assert_userdata(queue)
	local getqueue = void.queue.get("test_get_queue")
	lunatest.assert_userdata(getqueue)
	-- TODO: queue and getqueue should be address equal soon
	void.queue.destroy(queue)
	local getqueue2 = void.queue.get("test_get_queue")
	lunatest.assert_userdata(getqueue2)
	void.queue.destroy(getqueue)
	void.queue.destroy(getqueue2)
	lunatest.assert_nil(void.queue.get("test_get_queue"))
end

function suite.test_enqueue_await()
	local queue = void.queue.create(10, "test_enqueue")
	lunatest.assert_userdata(queue)
	local buffer = void.buffer.fromString "Hello!"
	lunatest.assert_userdata(buffer)
	lunatest.assert_true(void.queue.enqueue(queue, buffer))
	lunatest.assert_equal(void.queue.count(queue), 1)
	lunatest.assert_equal(void.buffer.type(buffer), "invalid")
	local outbuf = void.queue.await(queue)
	lunatest.assert_not_equal(buffer, outbuf)
	lunatest.assert_equal(void.buffer.asString(outbuf), "Hello!")
end

function suite.test_thread()
	local thread = require "llthreads2".new [[
		local void = require "void"
		local queue = void.queue.get "thread_queue"
		local buf = void.queue.await(queue, 1)
		print(void.buffer.asString(buf))
	]]

	local queue = void.queue.create(10, "thread_queue")
	thread:start(false, false)
	void.queue.enqueue(queue, void.buffer.fromString("Hey there!"))

	thread:join()
end

return suite
