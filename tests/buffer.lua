local void = require "void"

local suite = {}

function suite.test_create()
	local buffer = void.buffer.create(16)
	lunatest.assert_userdata(buffer)
	lunatest.assert_equal(void.buffer.length(buffer), 16)
	lunatest.assert_equal(void.buffer.type(buffer), "buffer")
end

function suite.test_create_from_string()
	local str = "Test buffer"
	local buffer = void.buffer.fromString(str)
	lunatest.assert_userdata(buffer)
	lunatest.assert_equal(void.buffer.length(buffer), #str)
	lunatest.assert_equal(void.buffer.asString(buffer), str)
	lunatest.assert_equal(void.buffer.getU8(buffer, 1), str:byte())

	local replaceChar = "A"
	local replacedStr = replaceChar..str:sub(2)
	void.buffer.setU8(buffer, 1, replaceChar:byte())
	lunatest.assert_equal(void.buffer.asString(buffer), replacedStr)
end

function suite.test_create_zero()
	local buffer = void.buffer.create(0)
	lunatest.assert_userdata(buffer)
	lunatest.assert_equal(void.buffer.length(buffer), 0)
	lunatest.assert_equal(void.buffer.asString(buffer), "")

	lunatest.assert_error(void.buffer.getU8, buffer, 1)
	lunatest.assert_error(void.buffer.setU8, buffer, 1, 0)
end

function suite.test_concat()
	local stra = "Hello, "
	local strb = "World!"
	local bufa = void.buffer.fromString(stra)
	local bufb = void.buffer.fromString(strb)
	lunatest.assert_userdata(bufa)
	lunatest.assert_userdata(bufb)
	lunatest.assert_equal(void.buffer.asString(bufa), stra)
	lunatest.assert_equal(void.buffer.asString(bufb), strb)

	local bufab = void.buffer.concat(bufa, bufb)
	lunatest.assert_userdata(bufab)
	lunatest.assert_equal(void.buffer.asString(bufab), stra..strb)
end

function suite.test_view()
	local str = "Hello, World!"
	local buffer = void.buffer.fromString(str)
	lunatest.assert_userdata(buffer)
	lunatest.assert_equal(void.buffer.asString(buffer), str)

	local viewstr = str:sub(1, 4)
	local view = void.buffer.view(buffer, 1, 4)
	lunatest.assert_userdata(view)
	lunatest.assert_equal(void.buffer.asString(view), viewstr)
	lunatest.assert_equal(void.buffer.type(view), "view")
end

function suite.test_clone()
	local buffer = void.buffer.fromString("Hello")
	lunatest.assert_userdata(buffer)
	local copy = void.buffer.clone(buffer)
	lunatest.assert_userdata(copy)

	lunatest.assert_equal(void.buffer.length(buffer), void.buffer.length(copy))
	lunatest.assert_equal(void.buffer.asString(buffer), void.buffer.asString(copy))

	void.buffer.setU8(copy, 5, ("!"):byte())
	lunatest.assert_not_equal(void.buffer.asString(buffer), void.buffer.asString(copy))
end

function suite.test_clone_range()
	local str = "Lets Test!"
	local strclone = str:sub(6, -2)
	local buffer = void.buffer.fromString(str)
	lunatest.assert_userdata(buffer)
	local copy = void.buffer.clone(buffer, 6, -2)
	lunatest.assert_userdata(copy)

	lunatest.assert_not_equal(void.buffer.asString(buffer), void.buffer.asString(copy))

	lunatest.assert_equal(void.buffer.asString(copy), strclone)
end

return suite
