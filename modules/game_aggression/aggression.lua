-- // USE:
--// doSendPlayerExtendedOpcode(cid, 8, "{'Name1', 'Name2', 'Name3', 'etc..'}")

aggressionWindow = nil
aggressionButton = nil
aggressionPanel = nil

aggressors = { }

function init()
  g_ui.importStyle('aggressionButton')
  aggressionButton = modules.client_topmenu.addRightGameToggleButton('aggressionButton', tr('Monster Aggression'), '/images/topbuttons/aggression', toggle)
  aggressionButton:setOn(true)
  aggressionWindow = g_ui.loadUI('aggression', modules.game_interface.getRightPanel())

  aggressionPanel = aggressionWindow:recursiveGetChildById('aggressionPanel')
  aggressionWindow:setContentMinimumHeight(80)
  aggressionWindow:setup()
  
  ProtocolGame.registerExtendedOpcode(ExtendedIds.AggressionMap, onAggressionChange)
end

function terminate()
  aggressionButton:destroy()
  aggressionWindow:destroy()
  
  aggressors = { }
end

function onAggressionChange(protocol, opcode, buffer)
    if not buffer then
        return
    end

    local values = loadstring("return " .. buffer)()
    if not values then
        return
    end
    
    if not(type(values) == "table") then
        return
    end
    
    removeAllAggressors()
    
    if #values == 0 then
        return
    end

    for _, name in pairs(values) do
        addAggressor(name)
    end
end

function toggle()
  if aggressionButton:isOn() then
    aggressionWindow:close()
    aggressionButton:setOn(false)
  else
    aggressionWindow:open()
    aggressionButton:setOn(true)
  end
end

function onMiniWindowClose()
  aggressionButton:setOn(false)
end

function addAggressor(name)
    local aggressionLabel = g_ui.createWidget('AggressionLabel', aggressionPanel)
    table.insert(aggressors, aggressionLabel)
    aggressionLabel:setText(name)
end

function removeAllAggressors()
  for i, v in pairs(aggressors) do
    v:destroy()
  end
  
  aggressors = { }
end


