-- FFI calls
-- polymeric 2026

---@meta

-- This file lists the skeleton of a C module, for documentation purposes.
-- Its contents are ignored by the actual Lua runtime because the
-- C module is loaded at a higher precedence.

local call = {}

function call.unsafe_invoke_rax_rcx_rdx(fn, rcx, rdx) end

return call
