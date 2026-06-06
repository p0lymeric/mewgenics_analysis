-- C function wrappers
-- polymeric 2026

local cffi = require("amoeba.c.ffi")
local cmem = require("amoeba.c.mem")

local ffi = {}

--

ffi.e_abi = cffi.e_abi
ffi.e_type = cffi.e_type

---@class CFunction
---@field addr integer
---@field p_rvalue integer
---@field pp_avalue integer
ffi.CFunction = {
    __gc = function(self)
        if self.addr ~= 0 then
            cffi.unsafe_delete_cif(self.addr)
            cmem.unsafe_free(self.p_rvalue)
            cmem.unsafe_free(self.pp_avalue)
            self.addr = 0
        end
    end
}
ffi.CFunction.__index = ffi.CFunction

---@param rtype ptype
---@param atypes ptype[]
---@param abi e_abi|nil
---@return CFunction
function ffi.CFunction:new(rtype, atypes, abi)
    abi = abi or ffi.e_abi.WIN64

    local addr = cffi.new_cif(abi, rtype, atypes)

    local p_rvalue, pp_avalue = cffi.alloc_rvalue_avalue_buffers(addr)

    local o = { addr = addr, p_rvalue = p_rvalue, pp_avalue = pp_avalue }
    setmetatable(o, ffi.CFunction)
    return o
end

---@param fp integer
---@param ... integer
---@return integer
function ffi.CFunction:unsafe_call(fp, ...)
    cffi.unsafe_call(self.addr, fp, self.p_rvalue, { ... }, self.pp_avalue)
    return self.p_rvalue
end

--

return ffi
