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

---@class e_type
---@field void CType
---@field uint8 CType
---@field sint8 CType
---@field uint16 CType
---@field sint16 CType
---@field uint32 CType
---@field sint32 CType
---@field uint64 CType
---@field sint64 CType
---@field float CType
---@field double CType
---@field pointer CType
-- ---@field longdouble CType
ffi.e_type = {}

---@class CType
ffi.CType = {}

---@param elements CType[]
---@return CType
function ffi.CType.make(elements) end

---@class CInterface
ffi.CInterface = {}

---@param rtype CType
---@param atypes CType[]
---@param abi e_abi|nil
---@return CInterface
function ffi.CInterface.make(rtype, atypes, abi) end

---@param fp integer
---@param ... integer|number
---@return integer|number|nil
function ffi.CInterface:unsafe_call(fp, ...) end

---@param module_name string
---@return integer
function ffi.get_module_handle(module_name) end

---@param h_module integer
---@param proc_name string
---@return integer
function ffi.get_proc_address(h_module, proc_name) end

return ffi
