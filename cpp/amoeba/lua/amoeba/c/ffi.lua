-- FFI calls
-- polymeric 2026

---@meta

-- This file lists the skeleton of a C module, for documentation purposes.
-- Its contents are ignored by the actual Lua runtime because the
-- C module is loaded at a higher precedence.

local ffi = {}

---@enum e_status
ffi.e_status = {
    OK = 0,
    BAD_TYPEDEF = 1,
    BAD_ABI = 2,
    BAD_ARGTYPE = 3,
}

---@enum e_abi
ffi.e_abi = {
    WIN64 = 1,
    GNUW64 = 2,
    -- DEFAULT_ABI = 1,
}

--- These are pointers at runtime but we assign arbitrary numbers for static analysis use
---@enum e_type
ffi.e_type = {
    void = 0,
    uint8 = 1,
    sint8 = 2,
    uint16 = 3,
    sint16 = 4,
    uint32 = 5,
    sint32 = 6,
    uint64 = 7,
    sint64 = 8,
    float = 9,
    double = 10,
    pointer = 11,
    longdouble = 12,
}

---@alias ptype e_type|integer

---@param elements ptype[]
---@return integer
function ffi.new_type(elements) end

---@param type_ integer
function ffi.unsafe_delete_type(type_) end

---@param abi e_abi
---@param rtype ptype
---@param atypes ptype[]
---@return integer
function ffi.new_cif(abi, rtype, atypes) end

---@param p_cif integer
function ffi.unsafe_delete_cif(p_cif) end

---@param p_cif integer
---@param fp integer
---@param p_rvalue integer
---@param avalue integer[]
---@param pp_avalue integer
function ffi.unsafe_call(p_cif, fp, p_rvalue, avalue, pp_avalue) end

---@param p_cif integer
---@return integer p_rvalue
---@return integer pp_avalue
function ffi.alloc_rvalue_avalue_buffers(p_cif) end

---@param module_name string
---@return integer
function ffi.get_module_handle(module_name) end

---@param h_module integer
---@param proc_name string
---@return integer
function ffi.get_proc_address(h_module, proc_name) end

return ffi
