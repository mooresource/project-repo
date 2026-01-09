local Players = game:GetService("Players")
local RunService = game:GetService("RunService")
local UIS = game:GetService("UserInputService")
local TweenService = game:GetService("TweenService")
local Camera = workspace.CurrentCamera
local LP = Players.LocalPlayer

-- CONFIG
_G.Config = {
    Aimbot = false, AimbotKey = nil, AimbotUseVisibleCheck = true,
    Smoothness = 20, Fov = 150, ShowFov = false, HitChance = 100,
    Esp = false, EspBoxes = true, EspNames = true, EspTeamMode = "Enemies",
    UseTeamCheck = true, EnemyColor = Color3.fromRGB(255, 68, 68),
    TeammateColor = Color3.fromRGB(68, 255, 136), EspHealthBar = true,
    EspDistance = true, EspTracers = true, EspHeadDot = true,
    TriggerBot = false, TriggerBotKey = nil, TriggerBotDelay = 0.1,
    MiscBhop = false, MiscSpeed = false, SpeedAmount = 30, SpeedKey = nil,
    MiscNoclip = false, NoclipKey = nil, InfiniteJump = false,
    FlyHack = false, FlyKey = nil, FlySpeed = 50,
    MiscThirdPerson = false, ThirdPersonDistance = 12, CameraFOV = 70,
    NoFog = false, FullBright = false, AntiLag = false, MenuKey = Enum.KeyCode.Insert
}

local state = {
    bindingKey = nil, aimbotKeyHeld = false, triggerBotKeyHeld = false,
    noclipParts = {}, originalSpeeds = {}, flyActive = false
}

-- UTILITIES
local Utilities = {}

function Utilities.isEnemy(p)
    if p == LP or not LP.Character or not p.Character then return false end
    if not _G.Config.UseTeamCheck then return true end
    local sameTeam = (LP.Character.Parent == p.Character.Parent)
    if _G.Config.EspTeamMode == "Enemies" then return not sameTeam
    elseif _G.Config.EspTeamMode == "Teammates" then return sameTeam
    else return true end
end

function Utilities.isVisibleForAimbot(targetChar)
    if not _G.Config.AimbotUseVisibleCheck or not targetChar or not LP.Character then return true end
    local origin = LP.Character:FindFirstChild("Head") or LP.Character:FindFirstChild("HumanoidRootPart")
    if not origin then return false end
    local targetHead = targetChar:FindFirstChild("Head")
    if not targetHead then return false end
    local rayParams = RaycastParams.new()
    rayParams.FilterDescendantsInstances = {LP.Character, targetChar}
    rayParams.FilterType = Enum.RaycastFilterType.Exclude
    return workspace:Raycast(origin.Position, targetHead.Position - origin.Position, rayParams) == nil
end

function Utilities.getMoveDirection()
    local dir = Vector3.zero
    local lookVec = Camera.CFrame.LookVector
    local rightVec = Camera.CFrame.RightVector
    if UIS:IsKeyDown(Enum.KeyCode.W) then dir = dir + lookVec end
    if UIS:IsKeyDown(Enum.KeyCode.S) then dir = dir - lookVec end
    if UIS:IsKeyDown(Enum.KeyCode.A) then dir = dir - rightVec end
    if UIS:IsKeyDown(Enum.KeyCode.D) then dir = dir + rightVec end
    return Vector3.new(dir.X, 0, dir.Z).Unit
end

function Utilities.create(class, props)
    local obj = Instance.new(class)
    for k, v in pairs(props) do obj[k] = v end
    return obj
end

-- MOVEMENT
local Movement = {}

function Movement.updateSpeed()
    if not LP.Character or not LP.Character:FindFirstChild("HumanoidRootPart") then return end
    local hrp = LP.Character.HumanoidRootPart
    local hum = LP.Character:FindFirstChild("Humanoid")
    if not hum then return end
    
    local speedActive = _G.Config.MiscSpeed and (_G.Config.SpeedKey == nil or UIS:IsKeyDown(_G.Config.SpeedKey))
    
    if speedActive and hum.MoveDirection.Magnitude > 0 then
        local speedMultiplier = _G.Config.SpeedAmount / 16
        local moveDirection = hum.MoveDirection
        hrp.CFrame = hrp.CFrame + (moveDirection * (speedMultiplier - 1) * 0.3)
    end
end

function Movement.setupNoclip()
    local PhysicsService = game:GetService("PhysicsService")
    
    pcall(function()
        if not PhysicsService:IsCollisionGroupRegistered("NoClipGroup") then
            PhysicsService:RegisterCollisionGroup("NoClipGroup")
        end
        PhysicsService:CollisionGroupSetCollidable("NoClipGroup", "Default", false)
    end)
end

function Movement.updateNoclip()
    if not LP.Character then return end
    local noclipActive = _G.Config.MiscNoclip and (_G.Config.NoclipKey == nil or UIS:IsKeyDown(_G.Config.NoclipKey))
    
    for _, part in pairs(LP.Character:GetDescendants()) do
        if part:IsA("BasePart") then
            if noclipActive then
                if not state.noclipParts[part] then
                    state.noclipParts[part] = part.CanCollide
                end
                part.CanCollide = false
                pcall(function()
                    game:GetService("PhysicsService"):SetPartCollisionGroup(part, "NoClipGroup")
                end)
            else
                if state.noclipParts[part] ~= nil then
                    part.CanCollide = state.noclipParts[part]
                    state.noclipParts[part] = nil
                    pcall(function()
                        game:GetService("PhysicsService"):SetPartCollisionGroup(part, "Default")
                    end)
                end
            end
        end
    end
end

function Movement.updateFly()
    if not LP.Character or not LP.Character:FindFirstChild("HumanoidRootPart") then
        state.flyActive = false
        return
    end
    
    local hrp = LP.Character.HumanoidRootPart
    local hum = LP.Character:FindFirstChild("Humanoid")
    if not hum then return end
    
    if _G.Config.FlyHack then
        state.flyActive = true
        hum.PlatformStand = true
        
        local move = Vector3.new()
        if UIS:IsKeyDown(Enum.KeyCode.W) then move = move + Camera.CFrame.LookVector end
        if UIS:IsKeyDown(Enum.KeyCode.S) then move = move - Camera.CFrame.LookVector end
        if UIS:IsKeyDown(Enum.KeyCode.A) then move = move - Camera.CFrame.RightVector end
        if UIS:IsKeyDown(Enum.KeyCode.D) then move = move + Camera.CFrame.RightVector end
        if UIS:IsKeyDown(Enum.KeyCode.Space) then move = move + Vector3.new(0, 1, 0) end
        if UIS:IsKeyDown(Enum.KeyCode.LeftShift) then move = move - Vector3.new(0, 1, 0) end
        
        if move.Magnitude > 0 then
            move = move.Unit * _G.Config.FlySpeed
            hrp.Velocity = move
        else
            hrp.Velocity = Vector3.new(0, 0, 0)
        end
        
        hrp.CFrame = CFrame.new(hrp.Position, hrp.Position + Camera.CFrame.LookVector)
    else
        if state.flyActive then
            hum.PlatformStand = false
            hrp.Velocity = Vector3.new(0, 0, 0)
            state.flyActive = false
        end
    end
end

function Movement.updateBhop()
    if not LP.Character or not LP.Character:FindFirstChild("Humanoid") or _G.Config.FlyHack then return end
    
    if _G.Config.MiscBhop and UIS:IsKeyDown(Enum.KeyCode.Space) then
        local hrp = LP.Character:FindFirstChild("HumanoidRootPart")
        local hum = LP.Character:FindFirstChild("Humanoid")
        if hrp and hum then
            local rayParams = RaycastParams.new()
            rayParams.FilterDescendantsInstances = {LP.Character}
            rayParams.FilterType = Enum.RaycastFilterType.Exclude
            local groundCheck = workspace:Raycast(hrp.Position, Vector3.new(0, -4, 0), rayParams)
            if groundCheck then hum.Jump = true end
            
            local success, moveDir = pcall(Utilities.getMoveDirection)
            if success and moveDir.Magnitude > 0 then
                local bhopSpeed = 18
                local smoothness = 0.2
                local velocity = hrp.AssemblyLinearVelocity
                local targetDir = moveDir * bhopSpeed
                local newX = velocity.X + (targetDir.X - velocity.X) * smoothness
                local newZ = velocity.Z + (targetDir.Z - velocity.Z) * smoothness
                hrp.AssemblyLinearVelocity = Vector3.new(newX, velocity.Y, newZ)
            end
        end
    end
end

function Movement.updateInfiniteJump()
    if _G.Config.InfiniteJump and UIS:IsKeyDown(Enum.KeyCode.Space) then
        local c = LP.Character
        if c and c:FindFirstChild("Humanoid") then c.Humanoid:ChangeState(Enum.HumanoidStateType.Jumping) end
    end
end

-- VISUALS
local Visuals = {}
local Lighting = game:GetService("Lighting")
local originalLightingSettings = {
    Brightness = Lighting.Brightness,
    ClockTime = Lighting.ClockTime,
    FogEnd = Lighting.FogEnd,
    GlobalShadows = Lighting.GlobalShadows,
    Ambient = Lighting.Ambient
}

function Visuals.updateFullBright()
    if _G.Config.FullBright then
        Lighting.Brightness = 2
        Lighting.ClockTime = 14
        Lighting.GlobalShadows = false
        Lighting.Ambient = Color3.fromRGB(178, 178, 178)
    else
        Lighting.Brightness = originalLightingSettings.Brightness
        Lighting.ClockTime = originalLightingSettings.ClockTime
        Lighting.GlobalShadows = originalLightingSettings.GlobalShadows
        Lighting.Ambient = originalLightingSettings.Ambient
    end
end

function Visuals.updateNoFog()
    if _G.Config.NoFog then
        Lighting.FogEnd = 100000
        for _, v in pairs(Lighting:GetDescendants()) do
            if v:IsA("Atmosphere") then v.Density = 0 end
        end
    else
        Lighting.FogEnd = originalLightingSettings.FogEnd
    end
end

function Visuals.updateAntiLag()
    local Terrain = workspace:FindFirstChildOfClass("Terrain")
    if _G.Config.AntiLag then
        settings().Rendering.QualityLevel = Enum.QualityLevel.Level01
        for _, v in pairs(workspace:GetDescendants()) do
            if v:IsA("ParticleEmitter") or v:IsA("Trail") or v:IsA("Smoke") or v:IsA("Fire") or v:IsA("Sparkles") then
                v.Enabled = false
            end
            if v:IsA("MeshPart") or v:IsA("UnionOperation") then
                v.Material = Enum.Material.SmoothPlastic
                v.CastShadow = false
            end
        end
        if Terrain then
            Terrain.WaterWaveSize = 0
            Terrain.WaterWaveSpeed = 0
            Terrain.WaterReflectance = 0
            Terrain.WaterTransparency = 0
        end
    end
end

function Visuals.updateThirdPerson()
    if _G.Config.MiscThirdPerson and LP.Character and LP.Character:FindFirstChild("HumanoidRootPart") and LP.Character:FindFirstChild("Humanoid") then
        local root = LP.Character.HumanoidRootPart
        local hum = LP.Character.Humanoid
        
        local distance = _G.Config.ThirdPersonDistance
        local cameraCFrame = Camera.CFrame
        
        local rayOrigin = root.Position
        local rayDirection = -cameraCFrame.LookVector * distance
        
        local rayParams = RaycastParams.new()
        rayParams.FilterDescendantsInstances = {LP.Character}
        rayParams.FilterType = Enum.RaycastFilterType.Exclude
        
        local rayResult = workspace:Raycast(rayOrigin, rayDirection, rayParams)
        local finalDistance = rayResult and (rayResult.Position - rayOrigin).Magnitude - 0.5 or distance
        
        local targetPos = root.Position - cameraCFrame.LookVector * finalDistance + Vector3.new(0, 1.5, 0)
        Camera.CFrame = CFrame.new(targetPos, root.Position + Vector3.new(0, 1.5, 0))
        
        hum.CameraOffset = Vector3.new(0, 0, 0)
    end
end

function Visuals.updateCameraFOV()
    if _G.Config.CameraFOV ~= 70 then
        Camera.FieldOfView = _G.Config.CameraFOV
    end
end

-- ESP
local ESP = {}
local ESP_Table = {}

function ESP.createESP()
    return {
        BoxTL = Drawing.new("Line"), BoxTR = Drawing.new("Line"), BoxBL = Drawing.new("Line"), BoxBR = Drawing.new("Line"),
        BoxTL2 = Drawing.new("Line"), BoxTR2 = Drawing.new("Line"), BoxBL2 = Drawing.new("Line"), BoxBR2 = Drawing.new("Line"),
        Name = Drawing.new("Text"), Distance = Drawing.new("Text"), HealthText = Drawing.new("Text"),
        HealthBar = Drawing.new("Line"), HealthBg = Drawing.new("Line"), Tracer = Drawing.new("Line"), Dot = Drawing.new("Circle")
    }
end

function ESP.update()
    for _, p in pairs(Players:GetPlayers()) do
        if not _G.Config.Esp or p == LP then
            if ESP_Table[p] then for _, d in pairs(ESP_Table[p]) do d.Visible = false end end
        else
            local show = Utilities.isEnemy(p) or (_G.Config.EspTeamMode ~= "Enemies")
            if show and p.Character and p.Character:FindFirstChild("HumanoidRootPart") and p.Character:FindFirstChild("Humanoid") and p.Character.Humanoid.Health > 0 then
                if not ESP_Table[p] then ESP_Table[p] = ESP.createESP() end
                local data = ESP_Table[p]
                local root = p.Character.HumanoidRootPart
                local head = p.Character:FindFirstChild("Head")
                local pos, vis = Camera:WorldToViewportPoint(root.Position)
                if vis and pos.Z > 0 then
                    local color = Utilities.isEnemy(p) and _G.Config.EnemyColor or _G.Config.TeammateColor
                    local distance = LP.Character and LP.Character:FindFirstChild("HumanoidRootPart") and (LP.Character.HumanoidRootPart.Position - root.Position).Magnitude or 0
                    local alpha = math.clamp(1 - (distance / 500), 0.3, 1)
                    local size = math.clamp(2200 / pos.Z, 16, 80)
                    local cornerLen = size * 0.25
                    
                    if _G.Config.EspBoxes then
                        local x, y = pos.X - size/2, pos.Y - size * 0.85
                        local w, h = size, size * 1.7
                        local thick = 2
                        data.BoxTL.Visible, data.BoxTL2.Visible = true, true
                        data.BoxTL.From, data.BoxTL.To = Vector2.new(x, y), Vector2.new(x + cornerLen, y)
                        data.BoxTL2.From, data.BoxTL2.To = Vector2.new(x, y), Vector2.new(x, y + cornerLen)
                        data.BoxTL.Color, data.BoxTL2.Color = color, color
                        data.BoxTL.Thickness, data.BoxTL2.Thickness = thick, thick
                        data.BoxTL.Transparency, data.BoxTL2.Transparency = alpha, alpha
                        data.BoxTR.Visible, data.BoxTR2.Visible = true, true
                        data.BoxTR.From, data.BoxTR.To = Vector2.new(x + w - cornerLen, y), Vector2.new(x + w, y)
                        data.BoxTR2.From, data.BoxTR2.To = Vector2.new(x + w, y), Vector2.new(x + w, y + cornerLen)
                        data.BoxTR.Color, data.BoxTR2.Color = color, color
                        data.BoxTR.Thickness, data.BoxTR2.Thickness = thick, thick
                        data.BoxTR.Transparency, data.BoxTR2.Transparency = alpha, alpha
                        data.BoxBL.Visible, data.BoxBL2.Visible = true, true
                        data.BoxBL.From, data.BoxBL.To = Vector2.new(x, y + h), Vector2.new(x + cornerLen, y + h)
                        data.BoxBL2.From, data.BoxBL2.To = Vector2.new(x, y + h - cornerLen), Vector2.new(x, y + h)
                        data.BoxBL.Color, data.BoxBL2.Color = color, color
                        data.BoxBL.Thickness, data.BoxBL2.Thickness = thick, thick
                        data.BoxBL.Transparency, data.BoxBL2.Transparency = alpha, alpha
                        data.BoxBR.Visible, data.BoxBR2.Visible = true, true
                        data.BoxBR.From, data.BoxBR.To = Vector2.new(x + w - cornerLen, y + h), Vector2.new(x + w, y + h)
                        data.BoxBR2.From, data.BoxBR2.To = Vector2.new(x + w, y + h - cornerLen), Vector2.new(x + w, y + h)
                        data.BoxBR.Color, data.BoxBR2.Color = color, color
                        data.BoxBR.Thickness, data.BoxBR2.Thickness = thick, thick
                        data.BoxBR.Transparency, data.BoxBR2.Transparency = alpha, alpha
                    else
                        data.BoxTL.Visible, data.BoxTL2.Visible = false, false
                        data.BoxTR.Visible, data.BoxTR2.Visible = false, false
                        data.BoxBL.Visible, data.BoxBL2.Visible = false, false
                        data.BoxBR.Visible, data.BoxBR2.Visible = false, false
                    end
                    
                    data.Name.Visible = _G.Config.EspNames
                    data.Name.Text = p.Name
                    data.Name.Size = math.clamp(size / 4, 13, 16)
                    data.Name.Center = true
                    data.Name.Outline = true
                    data.Name.OutlineColor = Color3.new(0, 0, 0)
                    data.Name.Color = Color3.new(1, 1, 1)
                    data.Name.Transparency = alpha
                    data.Name.Font = 2
                    data.Name.Position = Vector2.new(pos.X, pos.Y - size * 0.85 - 20)
                    
                    if _G.Config.EspHealthBar then
                        local hp = p.Character.Humanoid.Health / p.Character.Humanoid.MaxHealth
                        local barH = size * 1.7
                        local barX = pos.X - size/2 - 6
                        local barY = pos.Y - size * 0.85
                        data.HealthBg.Visible = true
                        data.HealthBg.From = Vector2.new(barX, barY)
                        data.HealthBg.To = Vector2.new(barX, barY + barH)
                        data.HealthBg.Color = Color3.new(0.1, 0.1, 0.1)
                        data.HealthBg.Thickness = 4
                        data.HealthBg.Transparency = 0.7
                        data.HealthBar.Visible = true
                        data.HealthBar.From = Vector2.new(barX, barY + barH * (1 - hp))
                        data.HealthBar.To = Vector2.new(barX, barY + barH)
                        data.HealthBar.Color = Color3.fromRGB(math.floor(255 * (1 - hp)), math.floor(200 * hp), 0)
                        data.HealthBar.Thickness = 4
                        data.HealthBar.Transparency = alpha
                        if hp < 1 then
                            data.HealthText.Visible = true
                            data.HealthText.Text = tostring(math.floor(hp * 100))
                            data.HealthText.Size = 11
                            data.HealthText.Center = false
                            data.HealthText.Outline = true
                            data.HealthText.OutlineColor = Color3.new(0, 0, 0)
                            data.HealthText.Color = Color3.new(1, 1, 1)
                            data.HealthText.Transparency = alpha
                            data.HealthText.Font = 2
                            data.HealthText.Position = Vector2.new(barX - 20, barY + barH * (1 - hp) - 6)
                        else
                            data.HealthText.Visible = false
                        end
                    else
                        data.HealthBar.Visible, data.HealthBg.Visible, data.HealthText.Visible = false, false, false
                    end
                    
                    if _G.Config.EspDistance then
                        data.Distance.Visible = true
                        data.Distance.Text = math.floor(distance) .. "m"
                        data.Distance.Size = math.clamp(size / 5, 11, 13)
                        data.Distance.Center = true
                        data.Distance.Outline = true
                        data.Distance.OutlineColor = Color3.new(0, 0, 0)
                        data.Distance.Color = Color3.fromRGB(200, 200, 200)
                        data.Distance.Transparency = alpha
                        data.Distance.Font = 2
                        data.Distance.Position = Vector2.new(pos.X, pos.Y + size * 0.85 + 8)
                    else
                        data.Distance.Visible = false
                    end
                    
                    if _G.Config.EspTracers then
                        data.Tracer.Visible = true
                        data.Tracer.From = Vector2.new(Camera.ViewportSize.X/2, Camera.ViewportSize.Y)
                        data.Tracer.To = Vector2.new(pos.X, pos.Y)
                        data.Tracer.Color = color
                        data.Tracer.Thickness = 1.5
                        data.Tracer.Transparency = alpha * 0.5
                    else
                        data.Tracer.Visible = false
                    end
                    
                    if _G.Config.EspHeadDot and head then
                        local hPos, hVis = Camera:WorldToViewportPoint(head.Position)
                        if hVis and hPos.Z > 0 then
                            data.Dot.Visible = true
                            data.Dot.Radius = math.clamp(size / 12, 3, 6)
                            data.Dot.Position = Vector2.new(hPos.X, hPos.Y)
                            data.Dot.Color = color
                            data.Dot.Filled = true
                            data.Dot.Transparency = alpha
                            data.Dot.Thickness = 1
                        else
                            data.Dot.Visible = false
                        end
                    else
                        data.Dot.Visible = false
                    end
                else
                    for _, d in pairs(data) do d.Visible = false end
                end
            elseif ESP_Table[p] then
                for _, d in pairs(ESP_Table[p]) do d.Visible = false end
            end
        end
    end
    
    for p, data in pairs(ESP_Table) do
        if not p or not p.Parent or not Players:FindFirstChild(p.Name) then
            for _, d in pairs(data) do d.Visible = false d:Remove() end
            ESP_Table[p] = nil
        end
    end
end

-- AIMBOT
local Aimbot = {}
local FovCircle = Drawing.new("Circle")
FovCircle.Color = Color3.fromRGB(255, 68, 68)
FovCircle.Thickness = 2
FovCircle.NumSides = 64
FovCircle.Transparency = 0.6
FovCircle.Filled = false

function Aimbot.updateFOV()
    FovCircle.Visible = _G.Config.ShowFov
    FovCircle.Radius = _G.Config.Fov
    FovCircle.Position = UIS:GetMouseLocation()
end

function Aimbot.update()
    local aimbotActive = _G.Config.Aimbot and (_G.Config.AimbotKey == nil or state.aimbotKeyHeld)
    if aimbotActive and mousemoverel then
        local target, dist = nil, _G.Config.Fov
        local mousePos = UIS:GetMouseLocation()
        for _, p in pairs(Players:GetPlayers()) do
            if Utilities.isEnemy(p) and p.Character and p.Character:FindFirstChild("Head") and p.Character.Humanoid.Health > 0 then
                if Utilities.isVisibleForAimbot(p.Character) then
                    local pos, vis = Camera:WorldToViewportPoint(p.Character.Head.Position)
                    if vis then
                        local mag = (Vector2.new(pos.X, pos.Y) - mousePos).Magnitude
                        if mag < dist and math.random(100) <= _G.Config.HitChance then dist, target = mag, p end
                    end
                end
            end
        end
        if target and target.Character:FindFirstChild("Head") then
            local targetPos = Camera:WorldToViewportPoint(target.Character.Head.Position)
            local currentPos = UIS:GetMouseLocation()
            local smooth = math.max(_G.Config.Smoothness / 10, 0.1)
            mousemoverel((targetPos.X - currentPos.X) / smooth, (targetPos.Y - currentPos.Y) / smooth)
        end
    end
end

-- TRIGGERBOT
local TriggerBot = {}

function TriggerBot.update()
    local triggerBotActive = _G.Config.TriggerBot and (_G.Config.TriggerBotKey == nil or state.triggerBotKeyHeld)
    if triggerBotActive then
        local mouse = LP:GetMouse()
        if mouse.Target and mouse.Target.Parent then
            local hum = mouse.Target.Parent:FindFirstChild("Humanoid")
            if hum and Utilities.isEnemy(Players:GetPlayerFromCharacter(hum.Parent)) and mouse1click then mouse1click() end
        end
    end
end

-- GUI
local GUI = {}

function GUI.init()
    local ScreenGui = Utilities.create("ScreenGui", {Parent = game:GetService("CoreGui"), ResetOnSpawn = false, ZIndexBehavior = Enum.ZIndexBehavior.Sibling})
    local Main = Utilities.create("Frame", {Parent = ScreenGui, Size = UDim2.new(0, 580, 0, 420), Position = UDim2.new(0.5, -290, 0.5, -210), BackgroundColor3 = Color3.fromRGB(15, 15, 17), BorderSizePixel = 0})
    Utilities.create("UICorner", {Parent = Main, CornerRadius = UDim.new(0, 6)})
    Utilities.create("UIStroke", {Parent = Main, Color = Color3.fromRGB(35, 35, 40), Thickness = 1})

    -- Top Bar
    local TopBar = Utilities.create("Frame", {Parent = Main, Size = UDim2.new(1, 0, 0, 42), BackgroundColor3 = Color3.fromRGB(20, 20, 23), BorderSizePixel = 0})
    Utilities.create("UICorner", {Parent = TopBar, CornerRadius = UDim.new(0, 6)})
    local TopBarFill = Utilities.create("Frame", {Parent = TopBar, Size = UDim2.new(1, 0, 0, 24), Position = UDim2.new(0, 0, 1, -24), BackgroundColor3 = Color3.fromRGB(20, 20, 23), BorderSizePixel = 0})
    
    -- Title on left with accent
    local AccentBar = Utilities.create("Frame", {Parent = TopBar, Size = UDim2.new(0, 3, 1, 0), Position = UDim2.new(0, 0, 0, 0), BackgroundColor3 = Color3.fromRGB(88, 101, 242), BorderSizePixel = 0})
    local TitleLabel = Utilities.create("TextLabel", {Parent = TopBar, Size = UDim2.new(0, 200, 1, 0), Position = UDim2.new(0, 15, 0, 0), Text = "PHANTOM.cc", TextColor3 = Color3.fromRGB(220, 220, 225), TextSize = 16, Font = Enum.Font.GothamBold, BackgroundTransparency = 1, TextXAlignment = Enum.TextXAlignment.Left})
    
    -- Close button
    local CloseBtn = Utilities.create("TextButton", {Parent = TopBar, Size = UDim2.new(0, 32, 0, 32), Position = UDim2.new(1, -38, 0.5, -16), BackgroundColor3 = Color3.fromRGB(25, 25, 28), Text = "✕", TextColor3 = Color3.fromRGB(140, 140, 145), TextSize = 16, Font = Enum.Font.GothamBold, BorderSizePixel = 0})
    Utilities.create("UICorner", {Parent = CloseBtn, CornerRadius = UDim.new(0, 4)})
    CloseBtn.MouseButton1Click:Connect(function() Main.Visible = false end)
    CloseBtn.MouseEnter:Connect(function() TweenService:Create(CloseBtn, TweenInfo.new(0.15), {BackgroundColor3 = Color3.fromRGB(220, 50, 50)}):Play() end)
    CloseBtn.MouseLeave:Connect(function() TweenService:Create(CloseBtn, TweenInfo.new(0.15), {BackgroundColor3 = Color3.fromRGB(25, 25, 28)}):Play() end)

    -- Sidebar
    local Sidebar = Utilities.create("Frame", {Parent = Main, Size = UDim2.new(0, 130, 1, -56), Position = UDim2.new(0, 8, 0, 48), BackgroundColor3 = Color3.fromRGB(18, 18, 21), BorderSizePixel = 0})
    Utilities.create("UICorner", {Parent = Sidebar, CornerRadius = UDim.new(0, 4)})
    Utilities.create("UIStroke", {Parent = Sidebar, Color = Color3.fromRGB(30, 30, 35), Thickness = 1})
    local SidebarLayout = Utilities.create("UIListLayout", {Parent = Sidebar, Padding = UDim.new(0, 4), HorizontalAlignment = Enum.HorizontalAlignment.Center, SortOrder = Enum.SortOrder.LayoutOrder})
    Utilities.create("UIPadding", {Parent = Sidebar, PaddingTop = UDim.new(0, 8), PaddingBottom = UDim.new(0, 8)})

    -- Content Area
    local ContentArea = Utilities.create("Frame", {Parent = Main, Size = UDim2.new(1, -150, 1, -56), Position = UDim2.new(0, 142, 0, 48), BackgroundColor3 = Color3.fromRGB(18, 18, 21), BorderSizePixel = 0})
    Utilities.create("UICorner", {Parent = ContentArea, CornerRadius = UDim.new(0, 4)})
    Utilities.create("UIStroke", {Parent = ContentArea, Color = Color3.fromRGB(30, 30, 35), Thickness = 1})

    -- Scrolling content
    local ContentScroll = Utilities.create("ScrollingFrame", {Parent = ContentArea, Size = UDim2.new(1, -16, 1, -16), Position = UDim2.new(0, 8, 0, 8), BackgroundTransparency = 1, BorderSizePixel = 0, ScrollBarThickness = 4, ScrollBarImageColor3 = Color3.fromRGB(88, 101, 242), CanvasSize = UDim2.new(0, 0, 0, 0), AutomaticCanvasSize = Enum.AutomaticSize.Y})
    
    local ContentLayout = Utilities.create("UIListLayout", {Parent = ContentScroll, Padding = UDim.new(0, 8), HorizontalAlignment = Enum.HorizontalAlignment.Left})

    local Tabs, currentTab = {}, nil

    local function CreateTab(name)
        local btn = Utilities.create("TextButton", {Parent = Sidebar, Size = UDim2.new(1, -16, 0, 34), BackgroundColor3 = Color3.fromRGB(22, 22, 26), BorderSizePixel = 0, Text = name, TextColor3 = Color3.fromRGB(130, 130, 135), TextSize = 11, Font = Enum.Font.GothamSemibold, AutoButtonColor = false})
        Utilities.create("UICorner", {Parent = btn, CornerRadius = UDim.new(0, 4)})
        
        local tabContainer = Utilities.create("Frame", {Parent = ContentScroll, Size = UDim2.new(1, 0, 0, 0), BackgroundTransparency = 1, Visible = false})
        local tabLayout = Utilities.create("UIListLayout", {Parent = tabContainer, Padding = UDim.new(0, 8)})
        
        tabLayout:GetPropertyChangedSignal("AbsoluteContentSize"):Connect(function() 
            tabContainer.Size = UDim2.new(1, 0, 0, tabLayout.AbsoluteContentSize.Y) 
        end)
        
        Tabs[name] = tabContainer
        
        btn.MouseEnter:Connect(function() 
            if currentTab ~= name then 
                TweenService:Create(btn, TweenInfo.new(0.15), {BackgroundColor3 = Color3.fromRGB(28, 28, 33)}):Play() 
            end 
        end)
        btn.MouseLeave:Connect(function() 
            if currentTab ~= name then 
                TweenService:Create(btn, TweenInfo.new(0.15), {BackgroundColor3 = Color3.fromRGB(22, 22, 26)}):Play() 
            end 
        end)
        btn.MouseButton1Click:Connect(function()
            for _, t in pairs(Sidebar:GetChildren()) do 
                if t:IsA("TextButton") then 
                    TweenService:Create(t, TweenInfo.new(0.15), {BackgroundColor3 = Color3.fromRGB(22, 22, 26), TextColor3 = Color3.fromRGB(130, 130, 135)}):Play() 
                end 
            end
            TweenService:Create(btn, TweenInfo.new(0.15), {BackgroundColor3 = Color3.fromRGB(88, 101, 242), TextColor3 = Color3.fromRGB(255, 255, 255)}):Play()
            for _, container in pairs(Tabs) do container.Visible = false end
            Tabs[name].Visible = true
            currentTab = name
        end)
        
        if not currentTab then
            btn.BackgroundColor3 = Color3.fromRGB(88, 101, 242)
            btn.TextColor3 = Color3.fromRGB(255, 255, 255)
            Tabs[name].Visible = true
            currentTab = name
        end
    end

    local function CreateSection(tab, title)
        local section = Utilities.create("Frame", {Parent = Tabs[tab], Size = UDim2.new(1, 0, 0, 26), BackgroundTransparency = 1})
        Utilities.create("TextLabel", {Parent = section, Size = UDim2.new(1, 0, 1, 0), Position = UDim2.new(0, 6, 0, 0), Text = title, TextColor3 = Color3.fromRGB(150, 150, 155), TextSize = 11, Font = Enum.Font.GothamBold, BackgroundTransparency = 1, TextXAlignment = Enum.TextXAlignment.Left, TextYAlignment = Enum.TextYAlignment.Center})
        Utilities.create("Frame", {Parent = section, Size = UDim2.new(1, -12, 0, 1), Position = UDim2.new(0, 6, 1, -1), BackgroundColor3 = Color3.fromRGB(35, 35, 40), BorderSizePixel = 0})
        return Tabs[tab]
    end

    local function CreateToggle(parent, name, key)
        local frame = Utilities.create("Frame", {Parent = parent, Size = UDim2.new(1, 0, 0, 32), BackgroundColor3 = Color3.fromRGB(22, 22, 26), BorderSizePixel = 0})
        Utilities.create("UICorner", {Parent = frame, CornerRadius = UDim.new(0, 4)})
        Utilities.create("UIStroke", {Parent = frame, Color = Color3.fromRGB(30, 30, 35), Thickness = 1})
        
        local toggle = Utilities.create("Frame", {Parent = frame, Size = UDim2.new(0, 36, 0, 18), Position = UDim2.new(0, 8, 0.5, -9), BackgroundColor3 = _G.Config[key] and Color3.fromRGB(88, 101, 242) or Color3.fromRGB(35, 35, 40), BorderSizePixel = 0})
        Utilities.create("UICorner", {Parent = toggle, CornerRadius = UDim.new(1, 0)})
        
        local knob = Utilities.create("Frame", {Parent = toggle, Size = UDim2.new(0, 14, 0, 14), Position = _G.Config[key] and UDim2.new(1, -16, 0.5, -7) or UDim2.new(0, 2, 0.5, -7), BackgroundColor3 = Color3.fromRGB(240, 240, 245), BorderSizePixel = 0})
        Utilities.create("UICorner", {Parent = knob, CornerRadius = UDim.new(1, 0)})
        
        Utilities.create("TextLabel", {Parent = frame, Size = UDim2.new(1, -56, 1, 0), Position = UDim2.new(0, 50, 0, 0), Text = name, TextColor3 = Color3.fromRGB(180, 180, 185), TextSize = 10, Font = Enum.Font.Gotham, BackgroundTransparency = 1, TextXAlignment = Enum.TextXAlignment.Left})
        
        local btn = Utilities.create("TextButton", {Parent = frame, Size = UDim2.new(1, 0, 1, 0), BackgroundTransparency = 1, Text = ""})
        btn.MouseButton1Click:Connect(function()
            _G.Config[key] = not _G.Config[key]
            TweenService:Create(toggle, TweenInfo.new(0.2), {BackgroundColor3 = _G.Config[key] and Color3.fromRGB(88, 101, 242) or Color3.fromRGB(35, 35, 40)}):Play()
            TweenService:Create(knob, TweenInfo.new(0.2), {Position = _G.Config[key] and UDim2.new(1, -16, 0.5, -7) or UDim2.new(0, 2, 0.5, -7)}):Play()
            if key == "MiscNoclip" then Movement.setupNoclip() end
            if key == "NoFog" then Visuals.updateNoFog() end
            if key == "FullBright" then Visuals.updateFullBright() end
            if key == "AntiLag" then Visuals.updateAntiLag() end
        end)
    end

    local function CreateSlider(parent, name, min, max, key)
        local frame = Utilities.create("Frame", {Parent = parent, Size = UDim2.new(1, 0, 0, 48), BackgroundColor3 = Color3.fromRGB(22, 22, 26), BorderSizePixel = 0})
        Utilities.create("UICorner", {Parent = frame, CornerRadius = UDim.new(0, 4)})
        Utilities.create("UIStroke", {Parent = frame, Color = Color3.fromRGB(30, 30, 35), Thickness = 1})
        
        Utilities.create("TextLabel", {Parent = frame, Size = UDim2.new(1, -60, 0, 18), Position = UDim2.new(0, 10, 0, 6), Text = name, TextColor3 = Color3.fromRGB(180, 180, 185), TextSize = 10, Font = Enum.Font.Gotham, BackgroundTransparency = 1, TextXAlignment = Enum.TextXAlignment.Left})
        
        local value = Utilities.create("TextLabel", {Parent = frame, Size = UDim2.new(0, 50, 0, 18), Position = UDim2.new(1, -58, 0, 6), Text = tostring(_G.Config[key]), TextColor3 = Color3.fromRGB(88, 101, 242), TextSize = 10, Font = Enum.Font.GothamBold, BackgroundTransparency = 1, TextXAlignment = Enum.TextXAlignment.Right})
        
        local bar = Utilities.create("TextButton", {Parent = frame, Size = UDim2.new(1, -20, 0, 4), Position = UDim2.new(0, 10, 1, -14), BackgroundColor3 = Color3.fromRGB(30, 30, 35), BorderSizePixel = 0, Text = "", AutoButtonColor = false})
        Utilities.create("UICorner", {Parent = bar, CornerRadius = UDim.new(1, 0)})
        
        local fill = Utilities.create("Frame", {Parent = bar, Size = UDim2.new((_G.Config[key] - min) / (max - min), 0, 1, 0), BackgroundColor3 = Color3.fromRGB(88, 101, 242), BorderSizePixel = 0})
        Utilities.create("UICorner", {Parent = fill, CornerRadius = UDim.new(1, 0)})
        
        local function update(x)
            local pos = math.clamp((x - bar.AbsolutePosition.X) / bar.AbsoluteSize.X, 0, 1)
            local val = math.floor(min + (max - min) * pos)
            _G.Config[key] = val
            value.Text = tostring(val)
            fill.Size = UDim2.new(pos, 0, 1, 0)
        end
        local dragging = false
        bar.MouseButton1Down:Connect(function(x) dragging = true update(x) end)
        bar.InputEnded:Connect(function(input) if input.UserInputType == Enum.UserInputType.MouseButton1 then dragging = false end end)
        bar.MouseMoved:Connect(function(x) if dragging then update(x) end end)
    end

    local function CreateDropdown(parent, name, key)
        local frame = Utilities.create("Frame", {Parent = parent, Size = UDim2.new(1, 0, 0, 32), BackgroundColor3 = Color3.fromRGB(22, 22, 26), BorderSizePixel = 0})
        Utilities.create("UICorner", {Parent = frame, CornerRadius = UDim.new(0, 4)})
        Utilities.create("UIStroke", {Parent = frame, Color = Color3.fromRGB(30, 30, 35), Thickness = 1})
        
        Utilities.create("TextLabel", {Parent = frame, Size = UDim2.new(0.5, 0, 1, 0), Position = UDim2.new(0, 10, 0, 0), Text = name, TextColor3 = Color3.fromRGB(180, 180, 185), TextSize = 10, Font = Enum.Font.Gotham, BackgroundTransparency = 1, TextXAlignment = Enum.TextXAlignment.Left})
        
        local btn = Utilities.create("TextButton", {Parent = frame, Size = UDim2.new(0, 88, 0, 22), Position = UDim2.new(1, -96, 0.5, -11), BackgroundColor3 = Color3.fromRGB(30, 30, 35), Text = _G.Config[key], TextColor3 = Color3.fromRGB(88, 101, 242), TextSize = 9, Font = Enum.Font.GothamBold, BorderSizePixel = 0})
        Utilities.create("UICorner", {Parent = btn, CornerRadius = UDim.new(0, 4)})
        
        local modes = {"Enemies", "Teammates", "All"}
        btn.MouseButton1Click:Connect(function()
            local idx = table.find(modes, _G.Config[key])
            idx = (idx % #modes) + 1
            _G.Config[key] = modes[idx]
            btn.Text = modes[idx]
        end)
    end

    local function CreateKeybind(parent, name, keyConfigKey)
        local frame = Utilities.create("Frame", {Parent = parent, Size = UDim2.new(1, 0, 0, 32), BackgroundColor3 = Color3.fromRGB(22, 22, 26), BorderSizePixel = 0})
        Utilities.create("UICorner", {Parent = frame, CornerRadius = UDim.new(0, 4)})
        Utilities.create("UIStroke", {Parent = frame, Color = Color3.fromRGB(30, 30, 35), Thickness = 1})
        
        Utilities.create("TextLabel", {Parent = frame, Size = UDim2.new(0.5, 0, 1, 0), Position = UDim2.new(0, 10, 0, 0), Text = name, TextColor3 = Color3.fromRGB(180, 180, 185), TextSize = 10, Font = Enum.Font.Gotham, BackgroundTransparency = 1, TextXAlignment = Enum.TextXAlignment.Left})
        
        local keyDisplay = Utilities.create("Frame", {Parent = frame, Size = UDim2.new(0, 88, 0, 22), Position = UDim2.new(1, -96, 0.5, -11), BackgroundColor3 = Color3.fromRGB(28, 28, 33), BorderSizePixel = 0})
        Utilities.create("UICorner", {Parent = keyDisplay, CornerRadius = UDim.new(0, 4)})
        Utilities.create("UIStroke", {Parent = keyDisplay, Color = Color3.fromRGB(45, 45, 52), Thickness = 1})
        
        local btn = Utilities.create("TextButton", {Parent = keyDisplay, Size = UDim2.new(1, -20, 1, 0), Position = UDim2.new(0, 0, 0, 0), BackgroundTransparency = 1, Text = _G.Config[keyConfigKey] and _G.Config[keyConfigKey].Name or "None", TextColor3 = Color3.fromRGB(200, 200, 205), TextSize = 9, Font = Enum.Font.GothamMedium, BorderSizePixel = 0})
        
        btn.MouseButton1Click:Connect(function()
            btn.Text = "..."
            btn.TextColor3 = Color3.fromRGB(255, 180, 80)
            state.bindingKey = {keyConfigKey = keyConfigKey, btn = btn}
        end)
        
        local clearBtn = Utilities.create("TextButton", {Parent = keyDisplay, Size = UDim2.new(0, 18, 1, 0), Position = UDim2.new(1, -18, 0, 0), BackgroundColor3 = Color3.fromRGB(35, 35, 40), Text = "×", TextColor3 = Color3.fromRGB(160, 160, 165), TextSize = 14, Font = Enum.Font.GothamBold, BorderSizePixel = 0})
        Utilities.create("UICorner", {Parent = clearBtn, CornerRadius = UDim.new(0, 4)})
        clearBtn.MouseEnter:Connect(function() TweenService:Create(clearBtn, TweenInfo.new(0.1), {BackgroundColor3 = Color3.fromRGB(220, 50, 50), TextColor3 = Color3.fromRGB(255, 255, 255)}):Play() end)
        clearBtn.MouseLeave:Connect(function() TweenService:Create(clearBtn, TweenInfo.new(0.1), {BackgroundColor3 = Color3.fromRGB(35, 35, 40), TextColor3 = Color3.fromRGB(160, 160, 165)}):Play() end)
        clearBtn.MouseButton1Click:Connect(function()
            _G.Config[keyConfigKey] = nil
            btn.Text = "None"
            btn.TextColor3 = Color3.fromRGB(200, 200, 205)
        end)
    end

    -- Create Tabs
    CreateTab("Aimbot")
    CreateTab("Visuals")
    CreateTab("Combat")
    CreateTab("Movement")
    CreateTab("Misc")

    -- Aimbot Tab
    CreateSection("Aimbot", "SETTINGS")
    CreateToggle(Tabs["Aimbot"], "Enable Aimbot", "Aimbot")
    CreateToggle(Tabs["Aimbot"], "Visibility Check", "AimbotUseVisibleCheck")
    CreateToggle(Tabs["Aimbot"], "Show FOV Circle", "ShowFov")
    CreateKeybind(Tabs["Aimbot"], "Aimbot Key", "AimbotKey")
    CreateSlider(Tabs["Aimbot"], "FOV Radius", 10, 400, "Fov")
    CreateSlider(Tabs["Aimbot"], "Smoothness", 1, 50, "Smoothness")
    CreateSlider(Tabs["Aimbot"], "Hit Chance", 0, 100, "HitChance")

    -- Visuals Tab
    CreateSection("Visuals", "ESP SETTINGS")
    CreateToggle(Tabs["Visuals"], "Enable ESP", "Esp")
    CreateDropdown(Tabs["Visuals"], "ESP Mode", "EspTeamMode")
    CreateToggle(Tabs["Visuals"], "Team Check", "UseTeamCheck")
    CreateToggle(Tabs["Visuals"], "Boxes", "EspBoxes")
    CreateToggle(Tabs["Visuals"], "Names", "EspNames")
    CreateToggle(Tabs["Visuals"], "Health Bars", "EspHealthBar")
    CreateToggle(Tabs["Visuals"], "Distance", "EspDistance")
    CreateToggle(Tabs["Visuals"], "Tracers", "EspTracers")
    CreateToggle(Tabs["Visuals"], "Head Dot", "EspHeadDot")
    
    CreateSection("Visuals", "WORLD")
    CreateToggle(Tabs["Visuals"], "No Fog", "NoFog")
    CreateToggle(Tabs["Visuals"], "Full Bright", "FullBright")
    CreateToggle(Tabs["Visuals"], "Anti Lag", "AntiLag")

    -- Combat Tab
    CreateSection("Combat", "TRIGGER BOT")
    CreateToggle(Tabs["Combat"], "Enable Trigger Bot", "TriggerBot")
    CreateKeybind(Tabs["Combat"], "Trigger Bot Key", "TriggerBotKey")
    CreateSlider(Tabs["Combat"], "Trigger Delay", 1, 100, "TriggerBotDelay")

    -- Movement Tab
    CreateSection("Movement", "MOVEMENT")
    CreateToggle(Tabs["Movement"], "Bunny Hop (Hold Space)", "MiscBhop")
    CreateToggle(Tabs["Movement"], "Speed Hack", "MiscSpeed")
    CreateKeybind(Tabs["Movement"], "Speed Key", "SpeedKey")
    CreateSlider(Tabs["Movement"], "Speed Amount", 16, 150, "SpeedAmount")
    CreateToggle(Tabs["Movement"], "NoClip", "MiscNoclip")
    CreateKeybind(Tabs["Movement"], "NoClip Key", "NoclipKey")
    CreateToggle(Tabs["Movement"], "Infinite Jump", "InfiniteJump")
    CreateToggle(Tabs["Movement"], "Fly Hack", "FlyHack")
    CreateKeybind(Tabs["Movement"], "Fly Key", "FlyKey")
    CreateSlider(Tabs["Movement"], "Fly Speed", 10, 150, "FlySpeed")

    -- Misc Tab
    CreateSection("Misc", "CAMERA")
    CreateToggle(Tabs["Misc"], "Third Person", "MiscThirdPerson")
    CreateSlider(Tabs["Misc"], "Third Person Distance", 5, 30, "ThirdPersonDistance")
    CreateSlider(Tabs["Misc"], "Camera FOV", 70, 120, "CameraFOV")

    -- Dragging functionality
    local dragging, dragStart, startPos
    TopBar.InputBegan:Connect(function(i)
        if i.UserInputType == Enum.UserInputType.MouseButton1 then
            dragging = true
            dragStart = i.Position
            startPos = Main.Position
        end
    end)
    UIS.InputEnded:Connect(function(i)
        if i.UserInputType == Enum.UserInputType.MouseButton1 then dragging = false end
    end)
    UIS.InputChanged:Connect(function(i)
        if dragging and i.UserInputType == Enum.UserInputType.MouseMovement then
            local delta = i.Position - dragStart
            Main.Position = UDim2.new(startPos.X.Scale, startPos.X.Offset + delta.X, startPos.Y.Scale, startPos.Y.Offset + delta.Y)
        end
    end)

    -- Key bindings
    UIS.InputBegan:Connect(function(i)
        if i.KeyCode == _G.Config.MenuKey then Main.Visible = not Main.Visible end
        if state.bindingKey and i.KeyCode ~= Enum.KeyCode.Unknown then
            _G.Config[state.bindingKey.keyConfigKey] = i.KeyCode
            state.bindingKey.btn.Text = i.KeyCode.Name
            state.bindingKey.btn.TextColor3 = Color3.fromRGB(200, 200, 205)
            state.bindingKey = nil
        end
        if _G.Config.AimbotKey and i.KeyCode == _G.Config.AimbotKey then state.aimbotKeyHeld = true end
        if _G.Config.TriggerBotKey and i.KeyCode == _G.Config.TriggerBotKey then state.triggerBotKeyHeld = true end
        if _G.Config.FlyKey and i.KeyCode == _G.Config.FlyKey then _G.Config.FlyHack = not _G.Config.FlyHack end
    end)

    UIS.InputEnded:Connect(function(i)
        if _G.Config.AimbotKey and i.KeyCode == _G.Config.AimbotKey then state.aimbotKeyHeld = false end
        if _G.Config.TriggerBotKey and i.KeyCode == _G.Config.TriggerBotKey then state.triggerBotKeyHeld = false end
    end)
end

-- INIT
Movement.setupNoclip()
GUI.init()
RunService.RenderStepped:Connect(function()
    Aimbot.updateFOV()
    Movement.updateFly()
    Movement.updateSpeed()
    Visuals.updateCameraFOV()
    Visuals.updateThirdPerson()
    Movement.updateInfiniteJump()
    Aimbot.update()
    ESP.update()
end)
RunService.Heartbeat:Connect(function()
    Movement.updateNoclip()
    Movement.updateBhop()
    TriggerBot.update()
end)
LP.CharacterAdded:Connect(function()
    wait(0.2)
    Movement.setupNoclip()
    state.noclipParts = {}
end)
