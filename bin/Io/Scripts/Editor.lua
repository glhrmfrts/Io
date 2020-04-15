file_selector = nil
def_style = nil
circuit_file = nil
ui_menubar = nil

function ExtractFilename(eventData, forSave)
    local fileName=""

    -- Check for OK
    if eventData["OK"]:GetBool() then
        local filter = eventData["Filter"]:GetString();
        fileName = eventData["FileName"]:GetString();
        -- Add default extension for saving if not specified
        if (GetExtension(fileName)=="" and forSave==true and filter ~= "*.*")
		then
            fileName = fileName..string.sub(filter,2)
		end
    end
    return fileName
end


------------------
-- File Selector
------------------

function HandleFileSelectorCancelPressed()
    print('CancelPressed')
    CloseFileSelector()
end

function CreateFileSelector(title, ok, cancel, initialPath, filters, initialFilter, autoLocalizeTitle)
    if file_selector then return false end

	if autoLocalizeTitle==nil then autoLocalizeTitle=true end

	if file_selector then
        UnsubscribeFromEvent(file_selector, "FileSelected")
	end

	file_selector=FileSelector:new()
	file_selector.defaultStyle=def_style
	file_selector.title=title
	file_selector.titleText.autoLocalizable=autoLocalizeTitle
	file_selector.path=initialPath
	file_selector:SetButtonTexts(ok, cancel)
	file_selector:SetFilters(filters, initialFilter)
	--CenterDialog(file_selector.window)
    file_selector.visible=true

    return true
end

function CloseFileSelector()
    if file_selector then
        UnsubscribeFromEvent(file_selector, "FileSelected")
    end
    file_selector.visible = false
    file_selector:delete()
    file_selector = nil
end

function CreateCircuitSelector()
    if CreateFileSelector("Select a circuit", "Load", "Cancel", fileSystem:GetCurrentDir(), {}, "", false) then
        SubscribeToEvent(file_selector, "FileSelected", "HandleCircuitFileSelected")
    end
end

function HandleCircuitFileSelected(event_type, event_data)
    circuit_file = ExtractFilename(event_data, false)
    event_data["circuit_file"] = circuit_file

    SendEvent("CircuitSelected", event_data)
    CloseFileSelector()
end


-----------------
-- Application
-----------------

function Start()
    -- Load default style
    def_style = cache:GetResource("XMLFile", "UI/DefaultStyle.xml")

    SubscribeToEvent("Update", "HandleUpdate")

    local data = VariantMap()
    data["circuit_file"] = "Io/DATA/BINARY/CIRCUITS/ALDERON.BL4"
    SendEvent("CircuitSelected", data)
end

function HandleUpdate()
    if input:GetKeyPress(KEY_RETURN) then
        CreateCircuitSelector()
    end
end

function Stop()

end