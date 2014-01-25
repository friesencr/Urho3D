const uint INPUT_SOURCE_KEYBOARD = 0;
const uint INPUT_SOURCE_MOUSE = 1;

Window@ keyboardBindingDialog;
ListView@ keybindingsListView;

// View bindings
InputBuilder@ inputCameraSpeedMultiplier = InputBuilder();
InputBuilder@ inputCameraForward = InputBuilder();
InputBuilder@ inputCameraBack = InputBuilder();
InputBuilder@ inputCameraLeft = InputBuilder();
InputBuilder@ inputCameraRight = InputBuilder();
InputBuilder@ inputCameraUp = InputBuilder();
InputBuilder@ inputCameraDown = InputBuilder();
InputBuilder@ inputCameraRotate = InputBuilder();
InputBuilder@ inputCameraOrbit = InputBuilder();
InputBuilder@ inputEditXAdd = InputBuilder();
InputBuilder@ inputEditXSub = InputBuilder();
InputBuilder@ inputEditYAdd = InputBuilder();
InputBuilder@ inputEditYSub = InputBuilder();
InputBuilder@ inputEditZAdd = InputBuilder();
InputBuilder@ inputEditZSub = InputBuilder();
InputBuilder@ inputEditUniformAdd = InputBuilder();
InputBuilder@ inputEditUniformSub = InputBuilder();
InputBuilder@ inputConsole = InputBuilder();
InputBuilder@ inputRenderingDebug = InputBuilder();
InputBuilder@ inputPhysicsDebug = InputBuilder();
InputBuilder@ inputOctreeDebug = InputBuilder();
InputBuilder@ inputCameraFrontView = InputBuilder();
InputBuilder@ inputCameraSideView = InputBuilder();
InputBuilder@ inputCameraTopView = InputBuilder();
InputBuilder@ inputEditModeMove = InputBuilder();
InputBuilder@ inputEditModeRotate = InputBuilder();
InputBuilder@ inputEditModeScale = InputBuilder();
InputBuilder@ inputEditModeSelect = InputBuilder();
InputBuilder@ inputAxisMode = InputBuilder();
InputBuilder@ inputPickModeUp = InputBuilder();
InputBuilder@ inputPickModeDown = InputBuilder();
InputBuilder@ inputFillMode = InputBuilder();
InputBuilder@ inputQuickMenu = InputBuilder();

void SetDefaultKeybindings()
{
    inputCameraSpeedMultiplier.Reset().Key(KEY_LSHIFT);
    inputCameraForward.Reset().Key(KEY_UP).Key('W');
    inputCameraBack.Reset().Key(KEY_DOWN).Key('S');
    inputCameraLeft.Reset().Key(KEY_LEFT).Key('A');
    inputCameraRight.Reset().Key(KEY_RIGHT).Key('D');
    inputCameraUp.Reset().Key(KEY_PAGEUP).Key('E');
    inputCameraDown.Reset().Key(KEY_PAGEDOWN).Key('Q');
    inputEditXAdd.Reset().Key(KEY_UP);
    inputEditXSub.Reset().Key(KEY_DOWN);
    inputEditYAdd.Reset().Key(KEY_LEFT);
    inputEditYSub.Reset().Key(KEY_RIGHT);
    inputEditZAdd.Reset().Key(KEY_PAGEUP);
    inputEditZSub.Reset().Key(KEY_PAGEDOWN);
    inputEditUniformAdd.Reset().Key(KEY_ADD);
    inputEditUniformSub.Reset().Key(KEY_SUBTRACT);
    inputConsole.Reset().Key(KEY_F1);
    inputRenderingDebug.Reset().Key(KEY_F2);
    inputPhysicsDebug.Reset().Key(KEY_F3);
    inputOctreeDebug.Reset().Key(KEY_F4);
    inputCameraFrontView.Reset().Key(KEY_NUMPAD1);
    inputCameraSideView.Reset().Key(KEY_NUMPAD3);
    inputCameraTopView.Reset().Key(KEY_NUMPAD7);
    inputEditModeMove.Reset().Key('1');
    inputEditModeRotate.Reset().Key('2');
    inputEditModeScale.Reset().Key('3');
    inputEditModeSelect.Reset().Key('4');
    inputAxisMode.Reset().Key('5');
    inputPickModeUp.Reset().Key('6');
    inputPickModeDown.Reset().Key('7');
    inputFillMode.Reset().Key('W');
    inputQuickMenu.Reset().Key(KEY_SPACE);
}

bool ShowEditorKeybindings()
{
    UpdateEditorKeybindingsDialog();
    keyboardBindingDialog.visible = true;
    keyboardBindingDialog.BringToFront();
    return true;
}

void HideEditorKeybindings()
{
    keyboardBindingDialog.visible = false;
}

void CreateEditorKeybindingsDialog()
{
    if (keyboardBindingDialog !is null)
        return;

    keyboardBindingDialog = ui.LoadLayout(cache.GetResource("XMLFile", "UI/EditorKeybindingsDialog.xml"));
    ui.root.AddChild(keyboardBindingDialog);

    keybindingsListView = keyboardBindingDialog.GetChild("BindingsList", true);
    UpdateEditorKeybindingsDialog();

    keyboardBindingDialog.opacity = uiMaxOpacity;
    keyboardBindingDialog.height = 440;
    CenterDialog(keyboardBindingDialog); 

    SubscribeToEvent(keyboardBindingDialog.GetChild("CloseButton", true), "Released", "HideEditorKeybindings");
}

void UpdateEditorKeybindingsDialog()
{
    for (uint i=0; i<bindingConfiguration.length; i++)
    {
        String key = bindingConfiguration.keys[i];
        InputBuilder@ binding;
        bool found = bindingConfiguration.Get(key, @binding);

        UIElement@ bindingRow = keybindingsListView.GetChild(key);
        if (bindingRow is null)
        {
            bindingRow = UIElement();
            bindingRow.style = AUTO_STYLE;
            bindingRow.SetLayout(LM_HORIZONTAL, 4);
            keybindingsListView.AddItem(bindingRow);
        }

        SetKeybindingListRow(bindingRow, key, binding);
    }
}

void SetKeybindingListRow(UIElement@ bindingRow, String key, InputBuilder InputBuilder)
{
    bindingRow.name = key;

    Text@ keyName;
    LineEdit@ binding1;
    LineEdit@ binding2;
    LineEdit@ binding3;

    if (bindingRow.numChildren == 0)
    {
        keyName = Text();
        keyName.style = AUTO_STYLE;
        keyName.width = 200;
        bindingRow.AddChild(keyName);

        binding1 = LineEdit();
        binding1.style = AUTO_STYLE;
        binding1.SetFixedWidth(75);
        bindingRow.AddChild(binding1);

        binding2 = LineEdit();
        binding2.style = AUTO_STYLE;
        binding2.SetFixedWidth(75);
        bindingRow.AddChild(binding2);

        binding3 = LineEdit();
        binding3.style = AUTO_STYLE;
        binding3.SetFixedWidth(75);
        bindingRow.AddChild(binding3);
    }
    else
    {
        Array<UIElement@> children = bindingRow.GetChildren();
        keyName = children[0];
        binding1 = children[1];
        binding2 = children[2];
        binding3 = children[3];
    }

    keyName.text = key;
    Array<InputBinding@>@ bindings = InputBuilder.bindings;
    // Print(key);
    // Print(bindings.length);
    if (bindings.length > 0)
        binding1.text = bindings[0].ToString();
    if (bindings.length > 1)
        binding2.text = bindings[1].ToString();
    if (bindings.length > 2)
        binding3.text = bindings[2].ToString();
}

Dictionary bindingConfiguration = {
	{ "camera_forward", inputCameraForward },
	{ "camera_back", inputCameraBack },
	{ "camera_left", inputCameraLeft },
	{ "camera_right", inputCameraRight },
    { "camera_speed_multiplier", inputCameraSpeedMultiplier },
    { "camera_forward", inputCameraForward },
    { "camera_back", inputCameraBack },
    { "camera_left", inputCameraLeft },
    { "camera_right", inputCameraRight },
    { "camera_up", inputCameraUp },
    { "camera_down", inputCameraDown },
    { "camera_front_view", inputCameraFrontView },
    { "camera_side_view", inputCameraSideView },
    { "camera_top_view", inputCameraTopView },
    { "edit_mode_x_add", inputEditXAdd },
    { "edit_mode_x_subtract", inputEditXSub },
    { "edit_mode_y_add", inputEditYAdd },
    { "edit_mode_y_subtract", inputEditYSub },
    { "edit_mode_z_add", inputEditZAdd },
    { "edit_mode_z_subtract", inputEditZSub },
    { "edit_mode_uniform_add", inputEditUniformAdd },
    { "edit_mode_uniform_subtract", inputEditUniformSub },
    { "edit_mode_move", inputEditModeMove },
    { "edit_mode_rotate", inputEditModeRotate },
    { "edit_mode_scale", inputEditModeScale },
    { "edit_mode_select", inputEditModeSelect },
    { "axis_mode", inputAxisMode },
    { "pick_mode_up", inputPickModeUp },
    { "pick_mode_down", inputPickModeDown },
    { "fill_mode", inputFillMode },
    { "quick_menu", inputQuickMenu },
    { "toggle_console", inputConsole },
    { "toggle_rendering_debug", inputRenderingDebug },
    { "toggle_physics_debug", inputPhysicsDebug },
    { "toggle_octree_debug", inputOctreeDebug }
};

Dictionary mouseButtonLookup = {
    { MOUSEB_LEFT, "LEFT" },
    { MOUSEB_RIGHT, "RIGHT" },
    { MOUSEB_MIDDLE, "MIDDLE" }
};

Dictionary qualifierKeyLookup = {
    { QUAL_SHIFT, "SHIFT" },
    { QUAL_CTRL, "CTRL" },
    { QUAL_ALT, "ALT" },
    { QUAL_ANY, "ANY" }
};

Dictionary keyboardKeyLookup = {
    { KEY_BACKSPACE, "BACKSPACE" },
    { KEY_TAB, "TAB" },
    { KEY_RETURN, "RETURN" },
    { KEY_RETURN2, "RETURN2" },
    { KEY_KP_ENTER, "KP_ENTER" },
    { KEY_PAUSE, "PAUSE" },
    { KEY_CAPSLOCK, "CAPSLOCK" },
    { KEY_ESC, "ESC" },
    { KEY_SPACE, "SPACE" },
    { KEY_PAGEUP, "PAGEUP" },
    { KEY_PAGEDOWN, "PAGEDOWN" },
    { KEY_END, "END" },
    { KEY_HOME, "HOME" },
    { KEY_LEFT, "LEFT" },
    { KEY_UP, "UP" },
    { KEY_RIGHT, "RIGHT" },
    { KEY_DOWN, "DOWN" },
    { KEY_INSERT, "INSERT" },
    { KEY_DELETE, "DELETE" },
    { KEY_LWIN, "LWIN" },
    { KEY_RWIN, "RWIN" },
    { KEY_APPS, "APPS" },
    { KEY_NUMPAD0, "NUMPAD0" },
    { KEY_NUMPAD1, "NUMPAD1" },
    { KEY_NUMPAD2, "NUMPAD2" },
    { KEY_NUMPAD3, "NUMPAD3" },
    { KEY_NUMPAD4, "NUMPAD4" },
    { KEY_NUMPAD5, "NUMPAD5" },
    { KEY_NUMPAD6, "NUMPAD6" },
    { KEY_NUMPAD7, "NUMPAD7" },
    { KEY_NUMPAD8, "NUMPAD8" },
    { KEY_NUMPAD9, "NUMPAD9" },
    { KEY_MULTIPLY, "MULTIPLY" },
    { KEY_ADD, "ADD" },
    { KEY_SUBTRACT, "SUBTRACT" },
    { KEY_DECIMAL, "DECIMAL" },
    { KEY_DIVIDE, "DIVIDE" },
    { KEY_F1, "F1" },
    { KEY_F2, "F2" },
    { KEY_F3, "F3" },
    { KEY_F4, "F4" },
    { KEY_F5, "F5" },
    { KEY_F6, "F6" },
    { KEY_F7, "F7" },
    { KEY_F8, "F8" },
    { KEY_F9, "F9" },
    { KEY_F10, "F10" },
    { KEY_F11, "F11" },
    { KEY_F12, "F12" },
    { KEY_F13, "F13" },
    { KEY_F14, "F14" },
    { KEY_F15, "F15" },
    { KEY_F16, "F16" },
    { KEY_F17, "F17" },
    { KEY_F18, "F18" },
    { KEY_F19, "F19" },
    { KEY_F20, "F20" },
    { KEY_F21, "F21" },
    { KEY_F22, "F22" },
    { KEY_F23, "F23" },
    { KEY_F24, "F24" },
    { KEY_NUMLOCK, "NUMLOCK" },
    { KEY_SCROLLLOCK, "SCROLLLOCK" },
    { KEY_LSHIFT, "LSHIFT" },
    { KEY_RSHIFT, "RSHIFT" },
    { KEY_LCTRL, "LCTRL" },
    { KEY_RCTRL, "RCTRL" },
    { KEY_LALT, "LALT" },
    { KEY_RALT, "RALT" }
};

class InputBinding
{
	int qualifiers = 0;
	int source = 0;
	int key = 0;

	InputBinding(int source_, int key_, int qualifiers_=0)
	{
		source = source_;
		key = key_;
		qualifiers = qualifiers_;
	}

	bool Down()
	{
		if (source == INPUT_SOURCE_KEYBOARD)
			return input.keyDown[key];
		else if (source == INPUT_SOURCE_MOUSE)
			return input.mouseButtonDown[key];
		else
			return false;
	}

	bool Press()
	{
		if (source == INPUT_SOURCE_KEYBOARD)
			return input.keyPress[key];
		else if (source == INPUT_SOURCE_MOUSE)
			return input.mouseButtonPress[key];
		else
			return false;
	}

    String ToString()
    {
        String str;
        if (source == INPUT_SOURCE_KEYBOARD)
        {
            if (!keyboardKeyLookup.Get(key, str))
            {
                str.AppendUTF8(key);
            }
        }
        return str;

    }
}

class InputBuilder
{
	Array<InputBinding@> bindings;

	InputBuilder() {}

	InputBuilder@ Key(int key, int qualifiers = 0)
	{
		InputBinding@ binding = InputBinding(0,key,qualifiers);
		bindings.Push(binding);
		return this;
	}

	InputBuilder@ Mouse(int key, int qualifiers = 0)
	{
		InputBinding@ binding = InputBinding(1,key,qualifiers);
		bindings.Push(binding);
		return this;
	}

    InputBuilder@ Reset()
    {
       bindings.Clear(); 
       return this;
    }

	bool Down()
	{
		for(uint i=0; i<bindings.length; i++)
		{
			if (bindings[i].Down())
				return true;
		}
		return false;
	}

	bool Press()
	{
		for(uint i=0; i<bindings.length; i++)
		{
			if (bindings[i].Press())
				return true;
		}
		return false;
	}
}