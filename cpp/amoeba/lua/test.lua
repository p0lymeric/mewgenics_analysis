-- Test script
-- polymeric 2026

local amoeba = require("amoeba")

local scenes = amoeba.glaiel.ecs.Director:new():get_scenes()
for k, _ in pairs(scenes) do
    if type(k) == "string" then
        print(k)
    end
end

print("")

for _, scene in ipairs(scenes) do
    print(scene:name())
    for i, entity in ipairs(scene:get_entities()) do
        print(string.format("    Entity %d:", i))
        for j, component in ipairs(entity:get_components()) do
            print(string.format("        %d", component:objid()))
        end
    end
end

for _, scene in ipairs(scenes) do
    print(scene:name())
    for i, component in ipairs(scene:get_components()) do
        print(string.format("    %d", component:objid()))
    end
end
