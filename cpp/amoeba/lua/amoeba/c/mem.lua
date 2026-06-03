-- Memory interfaces
-- polymeric 2026

---@meta

-- This file lists the skeleton of a C module, for documentation purposes.
-- Its contents are ignored by the actual Lua runtime because the
-- C module is loaded at a higher precedence.

local mem = {}

---@param addr integer
---@return integer
function mem.read_u8(addr) end
---@param addr integer
---@return integer
function mem.read_u16(addr) end
---@param addr integer
---@return integer
function mem.read_u32(addr) end
---@param addr integer
---@return integer
function mem.read_u64(addr) end
---@param addr integer
---@return integer
function mem.read_i8(addr) end
---@param addr integer
---@return integer
function mem.read_i16(addr) end
---@param addr integer
---@return integer
function mem.read_i32(addr) end
---@param addr integer
---@return integer
function mem.read_i64(addr) end
---@param addr integer
---@return number
function mem.read_f32(addr) end
---@param addr integer
---@return number
function mem.read_f64(addr) end

---@param addr integer
---@return integer
function mem.unsafe_read_u8(addr) end
---@param addr integer
---@return integer
function mem.unsafe_read_u16(addr) end
---@param addr integer
---@return integer
function mem.unsafe_read_u32(addr) end
---@param addr integer
---@return integer
function mem.unsafe_read_u64(addr) end
---@param addr integer
---@return integer
function mem.unsafe_read_i8(addr) end
---@param addr integer
---@return integer
function mem.unsafe_read_i16(addr) end
---@param addr integer
---@return integer
function mem.unsafe_read_i32(addr) end
---@param addr integer
---@return integer
function mem.unsafe_read_i64(addr) end
---@param addr integer
---@return number
function mem.unsafe_read_f32(addr) end
---@param addr integer
---@return number
function mem.unsafe_read_f64(addr) end

---@param size integer
---@return integer
function mem.alloc(size) end
---@param addr integer
function mem.unsafe_free(addr) end
---@param addr integer
---@param size integer
---@return integer
function mem.unsafe_realloc(addr, size) end
---@param addr integer
---@param value integer
---@param size integer
function mem.unsafe_memset(addr, value, size) end

return mem
