-- GON
-- polymeric 2026

local cmem = require("amoeba.c.mem")
local msvc = require("amoeba.msvc")

local gon = {}

local GONOBJECT_CHILDREN_MAP_OFFSET = 0
local GONOBJECT_CHILDREN_ARRAY_OFFSET = 56
local GONOBJECT_INT_DATA_OFFSET = 80
local GONOBJECT_FLOAT_DATA_OFFSET = 88
local GONOBJECT_BOOL_DATA_OFFSET = 92
local GONOBJECT_STRING_DATA_OFFSET = 104
local GONOBJECT_NAME_OFFSET = 136
local GONOBJECT_TYPE_OFFSET = 168
local GONOBJECT_SIZE = 176

---@enum GonFieldType
gon.GonFieldType = {
    NULLGON = 0,
    STRING = 1,
    NUMBER = 2,
    OBJECT = 3,
    ARRAY = 4,
    BOOL = 5,
}

gon.GonFieldTypeNames = {
    [0] = "NULLGON",
    [1] = "STRING",
    [2] = "NUMBER",
    [3] = "OBJECT",
    [4] = "ARRAY",
    [5] = "BOOL",
}

--

---@class GonObject
---@field addr integer
gon.GonObject = {}

---@param addr integer
---@return GonObject
function gon.GonObject:new(addr)
    local o = { addr = addr }
    setmetatable(o, self)
    self.__index = self
    return o
end

---@param max_size integer|nil
---@return string
function gon.GonObject:name(max_size)
    return msvc.xstring.read(self.addr + GONOBJECT_NAME_OFFSET, max_size)
end

---@return GonFieldType
function gon.GonObject:type()
    return cmem.read_i32(self.addr + GONOBJECT_TYPE_OFFSET)
end

---@return string
function gon.GonObject:type_str()
    return gon.GonFieldTypeNames[self:type()]
end

-- ---@return table<string, GonObject>
-- function gon.GonObject:object() end

-- ---@return GonObject[]
-- function gon.GonObject:array() end

---@param max_str_size integer|nil
---@param max_it_size integer|nil
---@return table<integer|string, GonObject>
function gon.GonObject:children(max_str_size, max_it_size)
    if max_it_size == nil then
        max_it_size = 0
    end

    local gonobjects = {}

    local vec_gonobjects = self.addr + GONOBJECT_CHILDREN_ARRAY_OFFSET
    local vec_first = cmem.read_u64(vec_gonobjects + 0)
    local vec_last = cmem.read_u64(vec_gonobjects + 8)
    local vec_size = (vec_last - vec_first) // GONOBJECT_SIZE

    if max_it_size ~= 0 and vec_size > max_it_size then
        error(string.format("vec_size %d exceeds max_it_size %d", vec_size, max_it_size))
    end

    for i = 0, vec_size - 1 do
        local gonobject = gon.GonObject:new(vec_first + i * GONOBJECT_SIZE)
        gonobjects[i + 1] = gonobject
    end

    local map = self.addr + GONOBJECT_CHILDREN_MAP_OFFSET
    local p_ctrls = cmem.read_u64(map + 0)
    local p_slots = cmem.read_u64(map + 8)
    local cap = cmem.read_u64(map + 24)

    if max_it_size ~= 0 and cap > max_it_size then
        error(string.format("hashmap cap %d exceeds max_it_size %d", cap, max_it_size))
    end

    for i = 0, cap - 1 do
        local ctrl = cmem.read_u8(p_ctrls + i)
        if ctrl <= 0x7F then
            local p_slot = p_slots + i * (32 + 4 + 4) -- std::string + int + pad(4)
            local key = msvc.xstring.read(p_slot + 0, max_str_size)
            local idx = cmem.read_i32(p_slot + 32)
            gonobjects[key] = gonobjects[idx + 1]
        end
    end

    return gonobjects
end

---@return integer
function gon.GonObject:int()
    return cmem.read_i32(self.addr + GONOBJECT_INT_DATA_OFFSET)
end

---@return number
function gon.GonObject:number()
    return cmem.read_f64(self.addr + GONOBJECT_FLOAT_DATA_OFFSET)
end

---@return boolean
function gon.GonObject:bool()
    return cmem.read_u8(self.addr + GONOBJECT_BOOL_DATA_OFFSET) ~= 0
end

---@param max_size integer|nil
---@return string
function gon.GonObject:string(max_size)
    return msvc.xstring.read(self.addr + GONOBJECT_STRING_DATA_OFFSET, max_size)
end

---@param max_str_size integer|nil
---@param max_it_size integer|nil
---@return integer|number|boolean|string|table<integer|string, GonObject>|nil
function gon.GonObject:get(max_str_size, max_it_size)
    local t = self:type()
    if t == gon.GonFieldType.NULLGON then return nil end
    if t == gon.GonFieldType.STRING then return self:string(max_str_size) end
    if t == gon.GonFieldType.NUMBER then return self:number() end
    if t == gon.GonFieldType.OBJECT then return self:children(max_str_size, max_it_size) end
    if t == gon.GonFieldType.ARRAY then return self:children(max_str_size, max_it_size) end
    if t == gon.GonFieldType.BOOL then return self:bool() end
end

--

return gon
