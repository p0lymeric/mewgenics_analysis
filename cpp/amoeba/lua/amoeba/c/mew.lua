-- Mewgenics process accessors
-- polymeric 2026

---@meta

-- This file lists the skeleton of a C module, for documentation purposes.
-- Its contents are ignored by the actual Lua runtime because the
-- C module is loaded at a higher precedence.

local mew = {}

---@return integer
function mew.get_mewdirector() end

---@param size integer
---@return integer
function mew.host_alloc(size) end

---@param addr integer
function mew.unsafe_host_free(addr) end

---@param addr integer
---@param size integer
---@return integer
function mew.unsafe_host_realloc(addr, size) end

return mew
