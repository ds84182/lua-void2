local void = require "void"

local suite = {}

function suite.test_void()
	lunatest.assert_table(void, "void is not table")
	lunatest.assert_table(void.buffer, "void.buffer is not table")
end

return suite
