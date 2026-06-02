-- MSVC std::string
-- polymeric 2026

local cmem = require("amoeba.c.mem")

local xstring = {}

function xstring.read(addr, max_size)
    if max_size == nil then
        max_size = 100
    end

    local size = cmem.read_u64(addr + 0x10)
    local capacity = cmem.read_u64(addr + 0x18)

    if max_size ~= 0 and size > max_size then
        error(string.format("string size %d exceeds max_size %d", size, max_size))
    end

    local p_buf = capacity < 16 and addr or cmem.read_u64(addr)

    local result = {}
    for i = 0, size - 1 do
        result[i + 1] = string.char(cmem.read_u8(p_buf + i))
    end

    return table.concat(result)
end

return xstring
