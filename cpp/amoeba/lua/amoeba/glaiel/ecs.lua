-- Mewgenics' ECS system
-- polymeric 2026

local ccall = require("amoeba.c.call")
local cmem = require("amoeba.c.mem")
local cmew = require("amoeba.c.mew")
local msvc = require("amoeba.msvc")

local ecs = {}

local COMPONENT_OBJID_OFFSET = 8;
local COMPONENT_DIRECTOR_OFFSET = 40
local COMPONENT_VTABLE_GETOBJECTTYPESTR_OFFSET = 0
local DIRECTOR_SCENES_OFFSET = 0
local SCENE_NAME_OFFSET = 1208
local SCENE_ENTITIES_OFFSET = 8
local SCENE_COMPONENTLISTS_OFFSET = 24
local ENTITY_COMPONENTS_OFFSET = 32

--

---@class MewDirector
---@field addr integer
ecs.MewDirector = {}

---@param addr integer|nil
---@return MewDirector
function ecs.MewDirector:new(addr)
    addr = addr or cmew.get_mewdirector()

    local o = { addr = addr }
    setmetatable(o, self)
    self.__index = self
    return o
end

--

---@class Director
---@field addr integer
ecs.Director = {}

---@param addr integer|nil
---@return Director
function ecs.Director:new(addr)
    addr = addr or cmem.read_u64(ecs.MewDirector:new().addr + COMPONENT_DIRECTOR_OFFSET)

    local o = { addr = addr }
    setmetatable(o, self)
    self.__index = self
    return o
end

---@return table<integer|string, Scene>
function ecs.Director:get_scenes()
    local scenes = {}

    local vec_scenes = self.addr + DIRECTOR_SCENES_OFFSET
    local vec_first = cmem.read_u64(vec_scenes + 0)
    local vec_last = cmem.read_u64(vec_scenes + 8)
    local vec_size = (vec_last - vec_first) // 8

    -- print(vec_first, vec_last, vec_last - vec_first, vec_size)

    for i = 0, vec_size - 1 do
        local scene = ecs.Scene:new(cmem.read_u64(vec_first + i * 8))
        scenes[i + 1] = scene
        scenes[scene:name()] = scene
    end

    return scenes
end

--

---@class Scene
---@field addr integer
ecs.Scene = {}

---@param addr integer
---@return Scene
function ecs.Scene:new(addr)
    local o = { addr = addr }
    setmetatable(o, self)
    self.__index = self
    return o
end

---@return string
function ecs.Scene:name()
    return msvc.xstring.read(self.addr + SCENE_NAME_OFFSET)
end

---@return Entity[]
function ecs.Scene:get_entities()
    local entities = {}

    local vec_entities = self.addr + SCENE_ENTITIES_OFFSET
    local vec_first = cmem.read_u64(vec_entities + 8)
    local vec_size = cmem.read_u32(vec_entities + 4)

    for i = 0, vec_size - 1 do
        local entity = ecs.Entity:new(cmem.read_u64(vec_first + i * 8))
        entities[i + 1] = entity
    end

    return entities
end

---@return table<integer|string, Component>
function ecs.Scene:get_components()
    local components = {}

    local vec_components = cmem.read_u64(self.addr + SCENE_COMPONENTLISTS_OFFSET)
    local vec_first = cmem.read_u64(vec_components + 8)
    local vec_size = cmem.read_u32(vec_components + 4)

    for i = 0, vec_size - 1 do
        local component = ecs.Component:new(cmem.read_u64(vec_first + i * 8))
        components[i + 1] = component
        components[component:get_object_type_str()] = component
    end

    return components
end

--

---@class Entity
---@field addr integer
ecs.Entity = {}

---@param addr integer
---@return Entity
function ecs.Entity:new(addr)
    local o = { addr = addr }
    setmetatable(o, self)
    self.__index = self
    return o
end

---@return table<integer|string, Component>
function ecs.Entity:get_components()
    local components = {}

    local vec_components = self.addr + ENTITY_COMPONENTS_OFFSET
    local vec_first = cmem.read_u64(vec_components + 8)
    local vec_size = cmem.read_u32(vec_components + 4)

    for i = 0, vec_size - 1 do
        local component = ecs.Component:new(cmem.read_u64(vec_first + i * 8))
        components[i + 1] = component
        components[component:get_object_type_str()] = component
    end

    return components
end

--

---@class Component
---@field addr integer
ecs.Component = {}

---@param addr integer
---@return Component
function ecs.Component:new(addr)
    local o = { addr = addr }
    setmetatable(o, self)
    self.__index = self
    return o
end

---@return integer
function ecs.Component:get_objid()
    return cmem.read_u32(self.addr + COMPONENT_OBJID_OFFSET)
end

---@return string
function ecs.Component:get_object_type_str()
    local vtable = cmem.read_u64(self.addr)
    local fn = cmem.read_u64(vtable + COMPONENT_VTABLE_GETOBJECTTYPESTR_OFFSET)

    local p_xstring = cmem.alloc(32)
    cmem.unsafe_memset(p_xstring, 0, 32)
    ccall.unsafe_invoke_rax_rcx_rdx(fn, self.addr, p_xstring)
    local str = msvc.xstring.read(p_xstring)
    local capacity = cmem.unsafe_read_u64(p_xstring + 0x18)
    if capacity >= 16 then
        cmem.unsafe_free(cmem.unsafe_read_u64(p_xstring))
    end
    cmem.unsafe_free(p_xstring)

    return str
end

--

return ecs
