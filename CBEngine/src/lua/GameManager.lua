-- GameManager base class
-- Inherit from this to create custom game managers:
--   MyManager = GameManager:Extend()

GameManager = {
    __fields = {},
    __isGameManager = true
}

function GameManager:Extend()
    local child = {}
    child.__fields = {}
    for k, v in pairs(self.__fields) do child.__fields[k] = v end
    setmetatable(child, { __index = self })
    child.__isGameManager = true
    return child
end

function GameManager:OnCreate() end
function GameManager:OnUpdate(dt) end
function GameManager:OnLateUpdate(dt) end
function GameManager:OnDestroy() end
