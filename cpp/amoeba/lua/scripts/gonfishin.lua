-- Test script: GON fishin'
-- polymeric 2026

local amoeba = require("amoeba")

local scenes = amoeba.glaiel.ecs.Director:new():get_scenes()
local spawn_database = scenes["Shared"]:get_components()["SpawnDatabase"]

print(string.format("p_SpawnDatabase: 0x%x", spawn_database.addr))

---@param gonobject GonObject
---@param depth integer|nil
---@param maxdepth integer|nil
local function walk_gon(offset, gonobject, depth, maxdepth)
    depth = depth or 0
    maxdepth = maxdepth or 1000000

    if depth >= maxdepth then
        return
    end

    local success, result = pcall(function()
        -- note we don't actually "need" a hardcoded bound on string or iterator sizes here
        -- as the internals of the script will stop upon reaching an unallocated page boundary
        -- just a "performance optimization" to avoid multi-second hitches
        gonobject:name(1000000)
        gonobject:get(1000000, 1000000)
        -- reject invalid types
        if gonobject:type_str() == nil then
            assert(false)
        end
        return gonobject
    end)
    if success then
        if depth == 0 then
            print(string.format("+0x%x", offset))
        end
        if result:type() == amoeba.glaiel.gon.GonFieldType.OBJECT or result:type() == amoeba.glaiel.gon.GonFieldType.ARRAY then
            print(string.format("%s%s %s", string.rep("    ", depth), result:type_str(), result:name()))
            pcall(function()
                local children = gonobject:children()
                for _, v in ipairs(children) do
                    walk_gon(offset, v, depth + 1, maxdepth)
                end
            end)
        else
            print(string.format("%s%s %s: %s", string.rep("    ", depth), result:type_str(), result:name(), result:get()))
        end
    else
        -- print("INVALID")
    end
end

for i = 0, 999999 do
    -- print(string.format("+0x%x", i * 8))
    walk_gon(i * 8, amoeba.glaiel.gon.GonObject:new(amoeba.c.mem.read_u64(spawn_database.addr + i * 8)))
end
