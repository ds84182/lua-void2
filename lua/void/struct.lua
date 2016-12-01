local struct = {}

local void = require "void.core"

local getter = {}
local setter = {}
local endians = {native = "", reverse = "re", little = "le", big = "be"}
local sizeof = {}

do
    for i, func in pairs(void.buffer) do
        if i:find("^get[USF]") then
            getter[i:sub(4):lower()] = func
        elseif i:find("^set[USF]") then
            setter[i:sub(4):lower()] = func
        end
    end
    
    local function typesiz(s, f, noed)
        local siz = math.floor(s/8) -- Keeps it as an int in Lua 5.3
        if f then
            sizeof["f"..s] = siz
        else
            sizeof["u"..s] = siz
            sizeof["s"..s] = siz
            if not noed then
                sizeof["u"..s.."le"] = siz
                sizeof["s"..s.."le"] = siz
                sizeof["u"..s.."be"] = siz
                sizeof["s"..s.."be"] = siz
                sizeof["u"..s.."re"] = siz
                sizeof["s"..s.."re"] = siz
            end
        end
    end
    
    typesiz(8, false, true)
    typesiz(16) typesiz(32) typesiz(64)
    typesiz(32, true) typesiz(64, true)
end

function struct.create(structdef)
    local endian = (structdef.endian or "native"):lower()
    local alignment = (structdef.alignment or 8)
    
    local function align(x)
        local y = alignment - (x % alignment)
        return (y == alignment and 0 or y)+x
    end
    
    assert(endians[endian], "Unknown endian: "..endian)
    assert(alignment >= 0, "Invalid alignment: "..alignment)
    
    local layout = {}
    local index = 0
    
    local function getLayoutFromType(field)
        local lyt, size
        local typ = field[2]:lower()
    
        if typ == "void" then
            --puts position in the resulting table
            lyt = {
                name = field[1], type = typ,
                function(buffer, index) return index end, nil, index, field[1]
            }
            size = 0
        elseif typ == "struct" then
            lyt = {
                name = field[1], type = typ, struct = field[3],
                function(buffer, index)
                    return struct.read(field[3], buffer, index)
                end,
                function(buffer, index, value)
                    struct.write(field[3], buffer, index, value)
                end, index, field[1]
            }
            size = field[3].size
        elseif typ == "array" then
            local count = field[3]
            local inner, innersize = getLayoutFromType(table.pack(table.unpack(field,3)))
            lyt = {
                name = field[1], type = typ,
                function(buffer, index)
                    local arr = {}
                    for i=1, count do
                        arr[i] = inner[1](buffer, index+((i-1)*innersize))
                    end
                    return arr
                end, function(buffer, index, value)
                    for i=1, count do
                        inner[2](buffer, index+((i-1)*innersize), value[i])
                    end
                end, index, field[1]
            }
            size = innersize*count
        else
            local typed = typ:sub(-2,-1)
        
            if typed == "ne" then
                typ = typ:sub(1,-3)
            end
    
            if typed ~= "le" and typed ~= "be" and typed ~= "re" and typed ~= "ne" then
                local ntyp = typ..endian
                if getter[ntyp] then
                    typ = ntyp
                end
            end
    
            if not getter[typ] then
                error("Invalid type for field "..field[1])
            end
    
            lyt = {
                name = field[1], type = typ,
                getter[typ], setter[typ], index, field[1]
            }
    
            size = sizeof[typ]
        end
        return lyt, size
    end
    
    for _, field in ipairs(structdef) do
        if field[2] == "inherit" then
            -- inherit fields from another struct
            local struct = field[3]
            local slayout = struct.layout
            for i=1, #slayout do
                local lyt = slayout[i]
                if layout[lyt.name] then error("Duplicate field "..lyt.name) end
                
                local nlyt = {}
                for i, v in pairs(lyt) do nlyt[i] = v end
                nlyt[3] = nlyt[3]+index
                
                layout[lyt.name] = nlyt
                layout[#layout+1] = nlyt
            end
            
            index = index+struct.size
        elseif field[2] == "padding" then
            index = index+field[3]
        else
            if layout[field[1]] then error("Duplicate field "..field[1]) end
        
            local lyt, siz = getLayoutFromType(field)
            index = index+siz
            layout[field[1]] = lyt
            layout[#layout+1] = lyt
        end
        
        index = align(index)
    end
    
    return {
        layout = layout,
        size = index
    }
end

function struct.read(struct, buffer, index)
    local dest = {}
    index = index or 0
    
    local layout = struct.layout
    for i=1, #layout do
        local field = layout[i]
        dest[field[4]] = field[1](buffer, index+field[3])
    end
    
    return dest
end

function struct.write(struct, buffer, index, src)
    if src == nil then
        if index == nil then
            src = buffer
            index = 0
            buffer = void.buffer.create(struct.size)
        else
            src = index
            index = 0
        end
    elseif index == nil then
        index = 0
    end
    
    local layout = struct.layout
    for i=1, #layout do
        local field = layout[i]
        if field[2] and src[field[4]] then
            field[2](buffer, index+field[3], src[field[4]])
        end
    end
    
    return buffer
end

return struct
