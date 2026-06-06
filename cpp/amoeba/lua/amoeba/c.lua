-- C(++) interface module
-- polymeric 2026

---@meta

-- This file lists the skeleton of a C module, for documentation purposes.
-- Its contents are ignored by the actual Lua runtime because the
-- C module is loaded at a higher precedence.

local c = {}

c.ffi = require("amoeba.c.ffi")
c.mem = require("amoeba.c.mem")
c.mew = require("amoeba.c.mew")

return c
