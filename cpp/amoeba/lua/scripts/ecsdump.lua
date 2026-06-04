-- Test script: ECS dump
-- polymeric 2026

local amoeba = require("amoeba")

local scenes = amoeba.glaiel.ecs.Director:new():get_scenes()

for _, scene in ipairs(scenes) do
    print(scene:name())
    for i, component in ipairs(scene:get_components()) do
        print(string.format("    %d: %s", component:get_objid(), component:get_object_type_str()))
    end
end
