InputBinding Binding(String key)
{
	InputBinding@ binding;
	if (bindingConfigurationDefaults.Get(key, @binding))
		return binding;
	else
		return emptyBinding;
}

const InputBinding emptyBinding;

Dictionary bindingConfigurationDefaults = {
	{ "camera_forward", InputBinding().Key(KEY_UP).Key('W') },
	{ "camera_back", InputBinding().Key(KEY_DOWN).Key('S') },
	{ "camera_left", InputBinding().Key(KEY_LEFT).Key('A') },
	{ "camera_right", InputBinding().Key(KEY_RIGHT).Key('D') }
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

Dictionary joystickHatLookup = {
    { HAT_CENTER, "CENTER" },
    { HAT_UP, "UP" },
    { HAT_RIGHT, "RIGHT" },
    { HAT_DOWN, "DOWN" },
    { HAT_LEFT, "LEFT" }
};

// class Binding
// {
// 	int qualifiers = 0;
// 	int source = 0;
// 	int key = 0;

// 	Binding(int source_, int key_, int qualifiers_=0)
// 	{
// 		source = source_;
// 		key = key_;
// 		qualifiers = qualifiers_;
// 	}
// 	bool Down()
// 	{
// 		return input.keyDown[key];
// 	}
// 	bool Press()
// 	{
// 		return input.keyPress[key];
// 	}
// }

class InputBinding
{
	Array<int> keyBindings;
	Array<int> mouseBindings;

	InputBinding() {}

	InputBinding@ Key(int binding)
	{
		keyBindings.Push(binding);
		return this;
	}

	InputBinding@ Mouse(int binding)
	{
		mouseBindings.Push(binding);
		return this;
	}

	bool Down()
	{
		for(uint i; i<keyBindings.length; i++)
		{
			Print(keyBindings[i]);
			if (input.keyDown[keyBindings[i]])
				return true;
		}

		for(uint i; i<mouseBindings.length; i++)
		{
			if (input.mouseButtonDown[mouseBindings[i]])
				return true;
		}
		return false;
	}

	bool Press()
	{
		for(uint i; i<keyBindings.length; i++)
		{
			Print(keyBindings[i]);
			if (input.keyPress[keyBindings[i]])
				return true;
		}

		for(uint i; i<mouseBindings.length; i++)
		{
			if (input.mouseButtonPress[mouseBindings[i]])
				return true;
		}
		return false;
	}

	bool Test() const 
	{
		return false;
	}
}
