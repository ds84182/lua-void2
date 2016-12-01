lunatest = require "lunatest"

lunatest.suite "require"
lunatest.suite "buffer"
lunatest.suite "queue"

--[[local void = require "void"

local buffer = void.buffer.create(16)
print(buffer, void.buffer.length(buffer))
local strbuf = void.buffer.fromString("Test buffer")
print(strbuf)
print(void.buffer.asString(strbuf))

local zero = void.buffer.create(0)

local bufa = void.buffer.fromString "Hello, "
local bufb = void.buffer.fromString "World!"
local bufab = void.buffer.concat(bufa, bufb)
print(void.buffer.asString(bufab))

local hell = void.buffer.view(bufab, 1, 4)
print(void.buffer.asString(hell))

print(void.buffer.type(bufab))
print(void.buffer.type(hell))

local hellcpy = void.buffer.duplicate(hell)
void.buffer.setU8(hellcpy, 1, ("I"):byte())
print(void.buffer.asString(hellcpy), void.buffer.asString(hell))

print(void.buffer.getU8(bufab, 1))
print(string.pack("I4",void.buffer.getU32(bufab, 1)))
print(string.pack("I4",void.buffer.getU32BE(bufab, 1)))

void.buffer.invalidate(bufab)
print(void.buffer.type(bufab))
print(void.buffer.type(hell))

local floatbuffer = void.buffer.create(4)
void.buffer.setF32BE(floatbuffer, 1, 1.5)
print(void.buffer.getF32(floatbuffer, 1))
print(void.buffer.getF32BE(floatbuffer, 1))]]

lunatest.run()
