-- Test script: Hexdump
-- polymeric 2026

local amoeba = require("amoeba")

local scenes = amoeba.glaiel.ecs.Director:new():get_scenes()

local function hexdump_at_home(addr, size)
    local function as_ascii_char(byte)
        return (byte >= 32 and byte <= 126) and string.char(byte) or "."
    end

    for offset = 0, size - 1, 16 do
        local row_size = math.min(size - offset, 16)
        local hex_str = ""
        local ascii_str = ""

        for i = 0, row_size - 1 do
            local byte = amoeba.c.mem.read_u8(addr + offset + i)
            hex_str = hex_str .. " "
            if i == 8 then
                hex_str = hex_str .. " "
            end
            hex_str = hex_str .. string.format("%02x", byte)
            ascii_str = ascii_str .. as_ascii_char(byte)
        end
        for i = row_size, 15 do
            if i == 8 then
                hex_str = hex_str .. " "
            end
            hex_str = hex_str .. "   "
            ascii_str = ascii_str .. " "
        end
        print(string.format("%016x: %s  %s", addr + offset, hex_str, ascii_str))
    end
end

hexdump_at_home(scenes["Shared"]:get_components()["GlobalProgressionData"].addr, 1337)
