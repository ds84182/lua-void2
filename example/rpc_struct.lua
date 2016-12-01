local void = require "void"

local struct = {}
struct.method = void.struct.create {
    {"method", "u32"},
    {"id", "u64"}
}

struct.method_kill = void.struct.create {
    {"", "inherit", struct.method}
}

struct.method_add = void.struct.create {
    {"", "inherit", struct.method},
    {"a", "f64"},
    {"b", "f64"}
}

return struct
