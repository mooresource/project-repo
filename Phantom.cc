--// Services
local Players = game:GetService("Players")
local RunService = game:GetService("RunService")
local Workspace = game:GetService("Workspace")
local UserInputService = game:GetService("UserInputService")
local TweenService = game:GetService("TweenService")

local LocalPlayer = Players.LocalPlayer
local Camera = Workspace.CurrentCamera
local PlayerGui = LocalPlayer:WaitForChild("PlayerGui")
local Mouse = LocalPlayer:GetMouse()

local Constants = {
    GUI_NAME = "FOVGui",
    GUI_SIZE = Vector2.new(400, 265),
    
    COLORS = {
        Background = Color3.fromRGB(20, 20, 25),
        GroupBox = Color3.fromRGB(30, 30, 35),
        GroupBoxBorder = Color3.fromRGB(60, 60, 70),
        Primary = Color3.fromRGB(100, 150, 255),
        Success = Color3.fromRGB(100, 200, 120),
        Text = Color3.fromRGB(200, 200, 210),
        TextDim = Color3.fromRGB(140, 140, 150),
        InputBg = Color3.fromRGB(25, 25, 30),
    },
    
    LIMITS = {
        FOVRadiusMin = 10,
        FOVRadiusMax = 800,
        MaxDistanceMin = 100,
        MaxDistanceMax = 10000,
        MissChanceMin = 0,
        MissChanceMax = 100,
    }
}

--// Configuration
local Config = {
    Enabled = true,
    FollowMouse = true,
    TeamCheck = false,
    WallCheck = false,
    ShowFOV = true,
    FOVRadius = 150,
    MaxDistance = 1000,
    TargetUsername = "",
    MissChance = 0,
    HitPart = "Head", -- Always Head
}

local Visual = {
    FOVColor = Color3.fromRGB(100, 150, 255),
    FOVThickness = 2,
    HighlightFill = Color3.fromRGB(255, 107, 107),
    HighlightOutline = Color3.fromRGB(100, 150, 255),
    FillTransparency = 0.6,
    OutlineTransparency = 0,
}

local State = {
    CurrentTarget = nil,
    CurrentHighlight = nil,
    Connections = {},
    ClickToTargetMode = false,
    FOVRingObj = nil,
    GUIVisible = true,
}

--// Cleanup
local function Cleanup()
    local oldGui = PlayerGui:FindFirstChild(Constants.GUI_NAME)
    if oldGui then oldGui:Destroy() end
    
    if State.CurrentHighlight then 
        State.CurrentHighlight:Destroy()
        State.CurrentHighlight = nil
    end

    for _, connection in pairs(State.Connections) do
        if connection then connection:Disconnect() end
    end
    State.Connections = {}
end

--// Utility Functions
local function AddConnection(name, connection)
    if State.Connections[name] then State.Connections[name]:Disconnect() end
    State.Connections[name] = connection
end

local function Clamp(value, min, max)
    return math.max(min, math.min(max, value))
end

local function IsVisible(targetPart)
    if not Config.WallCheck then return true end
    local ignoreList = {LocalPlayer.Character, targetPart.Parent}
    local parts = Camera:GetPartsObscuringTarget({targetPart.Position}, ignoreList)
    return #parts == 0
end

local function TweenObject(obj, props, duration)
    local tween = TweenService:Create(obj, TweenInfo.new(duration or 0.2, Enum.EasingStyle.Quad), props)
    tween:Play()
    return tween
end

local function GetFOVPosition()
    if Config.FollowMouse then
        local mousePos = UserInputService:GetMouseLocation()
        return Vector2.new(math.floor(mousePos.X + 0.5), math.floor(mousePos.Y + 0.5))
    else
        local viewport = Camera.ViewportSize
        return Vector2.new(math.floor(viewport.X / 2 + 0.5), math.floor(viewport.Y / 2 + 0.5))
    end
end

--// Target Logic
local function GetClosestPlayerToCursor()
    if not LocalPlayer.Character or not LocalPlayer.Character:FindFirstChild("HumanoidRootPart") then return nil end
    
    if Config.TargetUsername ~= "" then
        for _, Player in pairs(Players:GetPlayers()) do
            if Player ~= LocalPlayer then
                local matchesUsername = string.find(string.lower(Player.Name), string.lower(Config.TargetUsername))
                local matchesDisplay = string.find(string.lower(Player.DisplayName), string.lower(Config.TargetUsername))
                
                if matchesUsername or matchesDisplay then
                    if not Config.TeamCheck or Player.Team ~= LocalPlayer.Team then
                        local Character = Player.Character
                        if Character and Character:FindFirstChild("Head") and Character:FindFirstChild("Humanoid") and Character.Humanoid.Health > 0 then
                            local ScreenPos, OnScreen = Camera:WorldToScreenPoint(Character.Head.Position)
                            if OnScreen then return Player end
                        end
                    end
                end
            end
        end
        return nil
    end
    
    local ClosestPlayer, ShortestDistance = nil, Config.FOVRadius
    local OriginPos = GetFOVPosition()
    
    for _, Player in pairs(Players:GetPlayers()) do
        if Player ~= LocalPlayer then
            if not Config.TeamCheck or Player.Team ~= LocalPlayer.Team then
                local Character = Player.Character
                if Character and Character:FindFirstChild("Head") and Character:FindFirstChild("Humanoid") and Character.Humanoid.Health > 0 then
                    local Head = Character.Head
                    local Root = Character.HumanoidRootPart
                    local Distance = (LocalPlayer.Character.HumanoidRootPart.Position - Root.Position).Magnitude
                    
                    if Distance <= Config.MaxDistance and IsVisible(Head) then
                        local ScreenPos, OnScreen = Camera:WorldToScreenPoint(Head.Position)
                        if OnScreen then
                            local ScreenDist = (Vector2.new(ScreenPos.X, ScreenPos.Y) - OriginPos).Magnitude
                            if ScreenDist < ShortestDistance then
                                ClosestPlayer, ShortestDistance = Player, ScreenDist
                            end
                        end
                    end
                end
            end
        end
    end
    return ClosestPlayer
end

--// Highlight System
local Highlight = {}
function Highlight.Apply(player)
    if not player or not player.Character then return nil end
    local h = Instance.new("Highlight")
    h.FillColor = Visual.HighlightFill
    h.OutlineColor = Visual.HighlightOutline
    h.FillTransparency = Visual.FillTransparency
    h.OutlineTransparency = Visual.OutlineTransparency
    h.Adornee = player.Character 
    h.Parent = PlayerGui 
    return h
end

function Highlight.Remove()
    if State.CurrentHighlight then 
        State.CurrentHighlight:Destroy()
        State.CurrentHighlight = nil 
    end
end

function Highlight.Update(newTarget)
    if newTarget ~= State.CurrentTarget then
        Highlight.Remove()
        if newTarget then
            State.CurrentHighlight = Highlight.Apply(newTarget)
        end
    end
    State.CurrentTarget = newTarget
end

--// GUI Builder
local GUIBuilder = {}

function GUIBuilder.EnableClickToTarget(targetTextbox)
    State.ClickToTargetMode = true
    Mouse.Icon = "rbxasset://textures/Cursors/Crosshair.png"
    local connection
    connection = Mouse.Button1Down:Connect(function()
        local target = Mouse.Target
        if target then
            local character = target:FindFirstAncestorOfClass("Model")
            if character then
                local player = Players:GetPlayerFromCharacter(character)
                if player and player ~= LocalPlayer then
                    Config.TargetUsername = player.Name
                    targetTextbox.Text = player.Name
                    State.ClickToTargetMode = false
                    Mouse.Icon = ""
                    connection:Disconnect()
                end
            end
        end
    end)
    AddConnection("ClickToTarget", connection)
end

function GUIBuilder.CreateScreenGui()
    local ScreenGui = Instance.new("ScreenGui")
    ScreenGui.Name = Constants.GUI_NAME
    ScreenGui.ResetOnSpawn = false
    ScreenGui.IgnoreGuiInset = true
    ScreenGui.Parent = PlayerGui
    return ScreenGui
end

function GUIBuilder.CreateFOVRing(parent)
    local FOVRing = Instance.new("Frame")
    FOVRing.Size = UDim2.new(0, Config.FOVRadius * 2, 0, Config.FOVRadius * 2)
    FOVRing.AnchorPoint = Vector2.new(0.5, 0.5)
    FOVRing.BackgroundTransparency = 1
    FOVRing.Parent = parent
    
    local Stroke = Instance.new("UIStroke")
    Stroke.Thickness = Visual.FOVThickness
    Stroke.Color = Visual.FOVColor
    Stroke.Transparency = 0.3
    Stroke.Parent = FOVRing
    
    local Corner = Instance.new("UICorner")
    Corner.CornerRadius = UDim.new(1, 0)
    Corner.Parent = FOVRing
    
    State.FOVRingObj = FOVRing
    return FOVRing
end

function GUIBuilder.CreateMainFrame(parent)
    local MainFrame = Instance.new("Frame")
    MainFrame.Size = UDim2.new(0, Constants.GUI_SIZE.X, 0, Constants.GUI_SIZE.Y)
    MainFrame.Position = UDim2.new(0, 30, 0, 120)
    MainFrame.BackgroundColor3 = Constants.COLORS.Background
    MainFrame.BorderSizePixel = 0
    MainFrame.Parent = parent
    
    local Corner = Instance.new("UICorner")
    Corner.CornerRadius = UDim.new(0, 4)
    Corner.Parent = MainFrame
    
    local Stroke = Instance.new("UIStroke")
    Stroke.Color = Constants.COLORS.GroupBoxBorder
    Stroke.Thickness = 1
    Stroke.Parent = MainFrame
    
    -- Title Bar
    local TitleBar = Instance.new("Frame")
    TitleBar.Size = UDim2.new(1, 0, 0, 30)
    TitleBar.BackgroundTransparency = 1
    TitleBar.Parent = MainFrame
    
    local TitleLabel = Instance.new("TextLabel")
    TitleLabel.Size = UDim2.new(1, -60, 1, 0)
    TitleLabel.Position = UDim2.new(0, 10, 0, 0)
    TitleLabel.BackgroundTransparency = 1
    TitleLabel.Text = "SILENT AIM"
    TitleLabel.Font = Enum.Font.Code
    TitleLabel.TextSize = 14
    TitleLabel.TextColor3 = Constants.COLORS.Text
    TitleLabel.TextXAlignment = Enum.TextXAlignment.Left
    TitleLabel.Parent = TitleBar
    
    local MinimizeBtn = Instance.new("TextButton")
    MinimizeBtn.Size = UDim2.new(0, 20, 0, 20)
    MinimizeBtn.Position = UDim2.new(1, -30, 0, 5)
    MinimizeBtn.BackgroundColor3 = Constants.COLORS.InputBg
    MinimizeBtn.Text = "_"
    MinimizeBtn.Font = Enum.Font.Code
    MinimizeBtn.TextSize = 14
    MinimizeBtn.TextColor3 = Constants.COLORS.Text
    MinimizeBtn.BorderSizePixel = 0
    MinimizeBtn.Parent = TitleBar
    
    MinimizeBtn.MouseButton1Click:Connect(function()
        State.GUIVisible = not State.GUIVisible
        if State.GUIVisible then
            MainFrame.Size = UDim2.new(0, Constants.GUI_SIZE.X, 0, Constants.GUI_SIZE.Y)
            MinimizeBtn.Text = "_"
        else
            MainFrame.Size = UDim2.new(0, Constants.GUI_SIZE.X, 0, 30)
            MinimizeBtn.Text = "+"
        end
    end)

    return MainFrame
end

function GUIBuilder.CreateGroupBox(parent, title, position, size)
    local GroupBox = Instance.new("Frame")
    GroupBox.Size = size
    GroupBox.Position = position
    GroupBox.BackgroundColor3 = Constants.COLORS.GroupBox
    GroupBox.BorderSizePixel = 0
    GroupBox.Parent = parent
    
    local Corner = Instance.new("UICorner")
    Corner.CornerRadius = UDim.new(0, 3)
    Corner.Parent = GroupBox
    
    local Stroke = Instance.new("UIStroke")
    Stroke.Color = Constants.COLORS.GroupBoxBorder
    Stroke.Thickness = 1
    Stroke.Parent = GroupBox
    
    local TitleLabel = Instance.new("TextLabel")
    TitleLabel.Size = UDim2.new(1, -10, 0, 20)
    TitleLabel.Position = UDim2.new(0, 5, 0, 2)
    TitleLabel.BackgroundTransparency = 1
    TitleLabel.Text = title
    TitleLabel.Font = Enum.Font.Code
    TitleLabel.TextSize = 12
    TitleLabel.TextColor3 = Constants.COLORS.TextDim
    TitleLabel.TextXAlignment = Enum.TextXAlignment.Left
    TitleLabel.Parent = GroupBox
    
    local ContentFrame = Instance.new("Frame")
    ContentFrame.Size = UDim2.new(1, -10, 1, -25)
    ContentFrame.Position = UDim2.new(0, 5, 0, 22)
    ContentFrame.BackgroundTransparency = 1
    ContentFrame.Parent = GroupBox
    
    local UIList = Instance.new("UIListLayout")
    UIList.Padding = UDim.new(0, 5)
    UIList.Parent = ContentFrame
    
    return ContentFrame
end

function GUIBuilder.MakeDraggable(frame)
    local dragging, dragInput, dragStart, startPos
    
    frame.InputBegan:Connect(function(input)
        if input.UserInputType == Enum.UserInputType.MouseButton1 or input.UserInputType == Enum.UserInputType.Touch then
            dragging = true
            dragStart = input.Position
            startPos = frame.Position
            input.Changed:Connect(function()
                if input.UserInputState == Enum.UserInputState.End then dragging = false end
            end)
        end
    end)
    
    frame.InputChanged:Connect(function(input)
        if input.UserInputType == Enum.UserInputType.MouseMovement or input.UserInputType == Enum.UserInputType.Touch then dragInput = input end
    end)
    
    UserInputService.InputChanged:Connect(function(input)
        if input == dragInput and dragging then
            local delta = input.Position - dragStart
            frame.Position = UDim2.new(startPos.X.Scale, startPos.X.Offset + delta.X, startPos.Y.Scale, startPos.Y.Offset + delta.Y)
        end
    end)
end

function GUIBuilder.CreateCheckbox(parent, name, default, callback)
    local Container = Instance.new("Frame")
    Container.Size = UDim2.new(1, 0, 0, 18)
    Container.BackgroundTransparency = 1
    Container.Parent = parent
    
    local Checkbox = Instance.new("TextButton")
    Checkbox.Size = UDim2.new(0, 14, 0, 14)
    Checkbox.Position = UDim2.new(0, 0, 0, 2)
    Checkbox.BackgroundColor3 = Constants.COLORS.InputBg
    Checkbox.BorderSizePixel = 0
    Checkbox.Text = ""
    Checkbox.Parent = Container
    
    local CheckStroke = Instance.new("UIStroke")
    CheckStroke.Color = default and Constants.COLORS.Primary or Constants.COLORS.GroupBoxBorder
    CheckStroke.Thickness = 1
    CheckStroke.Parent = Checkbox
    
    local Checkmark = Instance.new("TextLabel")
    Checkmark.Size = UDim2.new(1, 0, 1, 0)
    Checkmark.BackgroundTransparency = 1
    Checkmark.Text = default and "✓" or ""
    Checkmark.Font = Enum.Font.Code
    Checkmark.TextSize = 12
    Checkmark.TextColor3 = Constants.COLORS.Primary
    Checkmark.Parent = Checkbox
    
    local Label = Instance.new("TextLabel")
    Label.Size = UDim2.new(1, -20, 1, 0)
    Label.Position = UDim2.new(0, 20, 0, 0)
    Label.BackgroundTransparency = 1
    Label.Text = name
    Label.Font = Enum.Font.Code
    Label.TextSize = 11
    Label.TextColor3 = Constants.COLORS.Text
    Label.TextXAlignment = Enum.TextXAlignment.Left
    Label.Parent = Container
    
    Checkbox.MouseButton1Click:Connect(function()
        default = not default
        Checkmark.Text = default and "✓" or ""
        CheckStroke.Color = default and Constants.COLORS.Primary or Constants.COLORS.GroupBoxBorder
        callback(default)
    end)
end

function GUIBuilder.CreateSlider(parent, name, min, max, default, callback)
    local Container = Instance.new("Frame")
    Container.Size = UDim2.new(1, 0, 0, 35)
    Container.BackgroundTransparency = 1
    Container.Parent = parent
    
    local Label = Instance.new("TextLabel")
    Label.Size = UDim2.new(0.6, 0, 0, 15)
    Label.BackgroundTransparency = 1
    Label.Text = name
    Label.Font = Enum.Font.Code
    Label.TextSize = 11
    Label.TextColor3 = Constants.COLORS.Text
    Label.TextXAlignment = Enum.TextXAlignment.Left
    Label.Parent = Container
    
    local ValueBox = Instance.new("TextBox")
    ValueBox.Size = UDim2.new(0, 50, 0, 15)
    ValueBox.Position = UDim2.new(1, -50, 0, 0)
    ValueBox.BackgroundColor3 = Constants.COLORS.InputBg
    ValueBox.BorderSizePixel = 0
    ValueBox.Font = Enum.Font.Code
    ValueBox.TextSize = 10
    ValueBox.TextColor3 = Constants.COLORS.Primary
    ValueBox.Text = tostring(default)
    ValueBox.Parent = Container
    
    local SliderBg = Instance.new("Frame")
    SliderBg.Size = UDim2.new(1, 0, 0, 4)
    SliderBg.Position = UDim2.new(0, 0, 1, -8)
    SliderBg.BackgroundColor3 = Constants.COLORS.InputBg
    SliderBg.BorderSizePixel = 0
    SliderBg.Parent = Container
    
    local SliderFill = Instance.new("Frame")
    SliderFill.Size = UDim2.new((default - min) / (max - min), 0, 1, 0)
    SliderFill.BackgroundColor3 = Constants.COLORS.Primary
    SliderFill.BorderSizePixel = 0
    SliderFill.Parent = SliderBg
    
    local dragging = false
    
    local function UpdateSlider(input)
        local pos = math.clamp((input.Position.X - SliderBg.AbsolutePosition.X) / SliderBg.AbsoluteSize.X, 0, 1)
        local value = math.floor(min + (max - min) * pos)
        SliderFill.Size = UDim2.new(pos, 0, 1, 0)
        ValueBox.Text = tostring(value)
        callback(value)
    end
    
    SliderBg.InputBegan:Connect(function(input)
        if input.UserInputType == Enum.UserInputType.MouseButton1 then
            dragging = true
            UpdateSlider(input)
        end
    end)
    
    SliderBg.InputEnded:Connect(function(input)
        if input.UserInputType == Enum.UserInputType.MouseButton1 then
            dragging = false
        end
    end)
    
    UserInputService.InputChanged:Connect(function(input)
        if dragging and input.UserInputType == Enum.UserInputType.MouseMovement then
            UpdateSlider(input)
        end
    end)
    
    ValueBox.FocusLost:Connect(function()
        local value = tonumber(ValueBox.Text) or default
        value = Clamp(value, min, max)
        ValueBox.Text = tostring(value)
        SliderFill.Size = UDim2.new((value - min) / (max - min), 0, 1, 0)
        callback(value)
    end)
end

function GUIBuilder.CreateCombobox(parent, name, options, default, callback)
    local Container = Instance.new("Frame")
    Container.Size = UDim2.new(1, 0, 0, 18)
    Container.BackgroundTransparency = 1
    Container.Parent = parent
    
    local Label = Instance.new("TextLabel")
    Label.Size = UDim2.new(0.5, 0, 1, 0)
    Label.BackgroundTransparency = 1
    Label.Text = name
    Label.Font = Enum.Font.Code
    Label.TextSize = 11
    Label.TextColor3 = Constants.COLORS.Text
    Label.TextXAlignment = Enum.TextXAlignment.Left
    Label.Parent = Container
    
    local Dropdown = Instance.new("TextButton")
    Dropdown.Size = UDim2.new(0, 80, 0, 16)
    Dropdown.Position = UDim2.new(1, -80, 0, 1)
    Dropdown.BackgroundColor3 = Constants.COLORS.InputBg
    Dropdown.BorderSizePixel = 0
    Dropdown.Font = Enum.Font.Code
    Dropdown.TextSize = 10
    Dropdown.TextColor3 = Constants.COLORS.Primary
    Dropdown.Text = default
    Dropdown.Parent = Container
    
    local isOpen = false
    local OptionsFrame = Instance.new("Frame")
    OptionsFrame.Size = UDim2.new(0, 80, 0, #options * 18)
    OptionsFrame.Position = UDim2.new(1, -80, 1, 2)
    OptionsFrame.BackgroundColor3 = Constants.COLORS.InputBg
    OptionsFrame.BorderSizePixel = 0
    OptionsFrame.Visible = false
    OptionsFrame.ZIndex = 10
    OptionsFrame.Parent = Container
    
    local OptionsList = Instance.new("UIListLayout")
    OptionsList.Parent = OptionsFrame
    
    for _, option in ipairs(options) do
        local OptionBtn = Instance.new("TextButton")
        OptionBtn.Size = UDim2.new(1, 0, 0, 18)
        OptionBtn.BackgroundColor3 = Constants.COLORS.GroupBox
        OptionBtn.BorderSizePixel = 0
        OptionBtn.Font = Enum.Font.Code
        OptionBtn.TextSize = 10
        OptionBtn.TextColor3 = Constants.COLORS.Text
        OptionBtn.Text = option
        OptionBtn.ZIndex = 11
        OptionBtn.Parent = OptionsFrame
        
        OptionBtn.MouseButton1Click:Connect(function()
            Dropdown.Text = option
            OptionsFrame.Visible = false
            isOpen = false
            callback(option)
        end)
    end
    
    Dropdown.MouseButton1Click:Connect(function()
        isOpen = not isOpen
        OptionsFrame.Visible = isOpen
    end)
end

function GUIBuilder.CreateTextbox(parent, name, default, callback, isTargetBox)
    local Container = Instance.new("Frame")
    Container.Size = UDim2.new(1, 0, 0, 18)
    Container.BackgroundTransparency = 1
    Container.Parent = parent
    
    local Label = Instance.new("TextLabel")
    Label.Size = UDim2.new(0, isTargetBox and 50 or 60, 1, 0)
    Label.BackgroundTransparency = 1
    Label.Text = name
    Label.Font = Enum.Font.Code
    Label.TextSize = 11
    Label.TextColor3 = Constants.COLORS.Text
    Label.TextXAlignment = Enum.TextXAlignment.Left
    Label.Parent = Container
    
    local InputBox = Instance.new("TextBox")
    InputBox.Size = UDim2.new(0, isTargetBox and 100 or 60, 0, 16)
    InputBox.Position = UDim2.new(1, isTargetBox and -130 or -60, 0, 1)
    InputBox.BackgroundColor3 = Constants.COLORS.InputBg
    InputBox.BorderSizePixel = 0
    InputBox.Font = Enum.Font.Code
    InputBox.TextSize = 10
    InputBox.TextColor3 = Constants.COLORS.Primary
    InputBox.Text = tostring(default)
    InputBox.Parent = Container
    
    if isTargetBox then
        local SelectBtn = Instance.new("TextButton")
        SelectBtn.Size = UDim2.new(0, 25, 0, 16)
        SelectBtn.Position = UDim2.new(1, -25, 0, 1)
        SelectBtn.BackgroundColor3 = Constants.COLORS.Primary
        SelectBtn.BorderSizePixel = 0
        SelectBtn.Font = Enum.Font.Code
        SelectBtn.TextSize = 9
        SelectBtn.TextColor3 = Color3.fromRGB(255, 255, 255)
        SelectBtn.Text = "SEL"
        SelectBtn.Parent = Container
        
        SelectBtn.MouseButton1Click:Connect(function()
            GUIBuilder.EnableClickToTarget(InputBox)
        end)
    end
    
    InputBox.FocusLost:Connect(function()
        callback(InputBox.Text)
    end)
end

--// Hooks
local function SetupHooks()
    if not hookmetamethod then return end

    local old_namecall
    old_namecall = hookmetamethod(game, "__namecall", function(self, ...)
        local args = {...}
        local method = getnamecallmethod()
        
        if Config.Enabled and State.CurrentTarget and method == "Raycast" and not checkcaller() then
            if typeof(args[1]) == "Vector3" and typeof(args[2]) == "Vector3" then
                local origin = args[1]
                local direction = args[2]
                local rayLength = direction.Magnitude
                
                local distanceFromCamera = (origin - Camera.CFrame.Position).Magnitude
                
                if rayLength > 10 or distanceFromCamera < 3 then
                    if Config.MissChance > 0 and math.random(1, 100) <= Config.MissChance then
                        return old_namecall(self, unpack(args))
                    end
                    
                    -- Always target Head
                    local targetPart = State.CurrentTarget.Character.Head
                    
                    args[2] = (targetPart.Position - origin).Unit * rayLength
                    return old_namecall(self, unpack(args))
                end
            end
        end
        return old_namecall(self, ...)
    end)
end

--// Initialize
local function Initialize()
    Cleanup()
    local ScreenGui = GUIBuilder.CreateScreenGui()
    local FOVRing = GUIBuilder.CreateFOVRing(ScreenGui)
    local MainFrame = GUIBuilder.CreateMainFrame(ScreenGui)
    GUIBuilder.MakeDraggable(MainFrame)
    
    -- Group Box 1: Toggles
    local TogglesGroup = GUIBuilder.CreateGroupBox(MainFrame, "TOGGLES", 
        UDim2.new(0, 10, 0, 35), UDim2.new(0, 185, 0, 145))
    
    GUIBuilder.CreateCheckbox(TogglesGroup, "Enabled", Config.Enabled, function(v) Config.Enabled = v end)
    GUIBuilder.CreateCheckbox(TogglesGroup, "Show FOV", Config.ShowFOV, function(v) Config.ShowFOV = v end)
    GUIBuilder.CreateCheckbox(TogglesGroup, "Follow Mouse", Config.FollowMouse, function(v) Config.FollowMouse = v end)
    GUIBuilder.CreateCheckbox(TogglesGroup, "Team Check", Config.TeamCheck, function(v) Config.TeamCheck = v end)
    GUIBuilder.CreateCheckbox(TogglesGroup, "Wall Check", Config.WallCheck, function(v) Config.WallCheck = v end)
    
    -- Group Box 2: Config
    local ConfigGroup = GUIBuilder.CreateGroupBox(MainFrame, "CONFIG", 
        UDim2.new(0, 205, 0, 35), UDim2.new(0, 185, 0, 145))
    
    GUIBuilder.CreateSlider(ConfigGroup, "FOV Radius", Constants.LIMITS.FOVRadiusMin, 
        Constants.LIMITS.FOVRadiusMax, Config.FOVRadius, function(v) Config.FOVRadius = v end)
    
    GUIBuilder.CreateSlider(ConfigGroup, "Max Distance", Constants.LIMITS.MaxDistanceMin, 
        Constants.LIMITS.MaxDistanceMax, Config.MaxDistance, function(v) Config.MaxDistance = v end)
    
    GUIBuilder.CreateSlider(ConfigGroup, "Miss Chance %", Constants.LIMITS.MissChanceMin, 
        Constants.LIMITS.MissChanceMax, Config.MissChance, function(v) Config.MissChance = v end)
    
    -- Group Box 3: Target
    local TargetGroup = GUIBuilder.CreateGroupBox(MainFrame, "TARGET", 
        UDim2.new(0, 10, 0, 190), UDim2.new(0, 380, 0, 45))
    
    GUIBuilder.CreateTextbox(TargetGroup, "Username", Config.TargetUsername, 
        function(v) Config.TargetUsername = v end, true)
    
    -- Main Loop
    local RunConnection = RunService.RenderStepped:Connect(function()
        local OriginPos = GetFOVPosition()
        
        if State.FOVRingObj then
            State.FOVRingObj.Visible = Config.ShowFOV
            State.FOVRingObj.Size = UDim2.new(0, Config.FOVRadius * 2, 0, Config.FOVRadius * 2)
            State.FOVRingObj.Position = UDim2.new(0, OriginPos.X, 0, OriginPos.Y)
        end
        
        if Config.Enabled then
            local target = GetClosestPlayerToCursor()
            Highlight.Update(target)
        else
            Highlight.Update(nil)
        end
    end)
    AddConnection("MainLoop", RunConnection)
    
    SetupHooks()
end

--// Start
Initialize()
