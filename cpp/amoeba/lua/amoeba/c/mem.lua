-- Memory interfaces
-- polymeric 2026

---@meta

-- This file lists the skeleton of a C module, for documentation purposes.
-- Its contents are ignored by the actual Lua runtime because the
-- C module is loaded at a higher precedence.

local mem = {}

function mem.read_u8(addr) end
function mem.read_u16(addr) end
function mem.read_u32(addr) end
function mem.read_u64(addr) end
function mem.read_i8(addr) end
function mem.read_i16(addr) end
function mem.read_i32(addr) end
function mem.read_i64(addr) end
function mem.read_f32(addr) end
function mem.read_f64(addr) end

return mem
