// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/InputNormalizer.h"
#include "usb_hid_keys.h"
#include "vkcodes.h"

FKey InputNormalizer::Normalize(int KeyCode, KeySource Source, int CharCode)
{
	// normalize to unreal key
	if(Source == ScanCode)
	{
		switch(KeyCode)
		{
			case SCANCODE_0: return EKeys::Zero;
			case SCANCODE_1: return EKeys::One;
			case SCANCODE_2: return EKeys::Two;
			case SCANCODE_3: return EKeys::Three;
			case SCANCODE_4: return EKeys::Four;
			case SCANCODE_5: return EKeys::Five;
			case SCANCODE_6: return EKeys::Six;
			case SCANCODE_7: return EKeys::Seven;
			case SCANCODE_8: return EKeys::Eight;
			case SCANCODE_9: return EKeys::Nine;
			case SCANCODE_A: return EKeys::A;
			case SCANCODE_B: return EKeys::B;
			case SCANCODE_C: return EKeys::C;
			case SCANCODE_D: return EKeys::D;
			case SCANCODE_E: return EKeys::E;
			case SCANCODE_F: return EKeys::F;
			case SCANCODE_G: return EKeys::G;
			case SCANCODE_H: return EKeys::H;
			case SCANCODE_I: return EKeys::I;
			case SCANCODE_J: return EKeys::J;
			case SCANCODE_K: return EKeys::K;
			case SCANCODE_L: return EKeys::L;
			case SCANCODE_M: return EKeys::M;
			case SCANCODE_N: return EKeys::N;
			case SCANCODE_O: return EKeys::O;
			case SCANCODE_P: return EKeys::P;
			case SCANCODE_Q: return EKeys::Q;
			case SCANCODE_R: return EKeys::R;
			case SCANCODE_S: return EKeys::S;
			case SCANCODE_T: return EKeys::T;
			case SCANCODE_U: return EKeys::U;
			case SCANCODE_V: return EKeys::V;
			case SCANCODE_W: return EKeys::W;
			case SCANCODE_X: return EKeys::X;
			case SCANCODE_Y: return EKeys::Y;
			case SCANCODE_Z: return EKeys::Z;
			case SCANCODE_APOSTROPHE: return EKeys::Apostrophe;
			case SCANCODE_BACKSLASH: return EKeys::Backslash;
			case SCANCODE_COMMA: return EKeys::Comma;
			case SCANCODE_EQUAL: return EKeys::Equals;
			case SCANCODE_GRAVE: return EKeys::Tilde;
			case SCANCODE_LEFTBRACE: return EKeys::LeftBracket;
			case SCANCODE_MINUS: return EKeys::Hyphen;
			case SCANCODE_DOT: return EKeys::Period;
			case SCANCODE_RIGHTBRACE: return EKeys::RightBracket;
			case SCANCODE_SEMICOLON: return EKeys::Semicolon;
			case SCANCODE_SLASH: return EKeys::Slash;
			case SCANCODE_BACKSPACE: return EKeys::BackSpace;
			case SCANCODE_CAPSLOCK: return EKeys::CapsLock;
			case SCANCODE_DELETE: return EKeys::Delete;
			case SCANCODE_DOWN: return EKeys::Down;
			case SCANCODE_END: return EKeys::End;
			case SCANCODE_ESC: return EKeys::Escape;
			case SCANCODE_F1: return EKeys::F1;
			case SCANCODE_F2: return EKeys::F2;
			case SCANCODE_F3: return EKeys::F3;
			case SCANCODE_F4: return EKeys::F4;
			case SCANCODE_F5: return EKeys::F5;
			case SCANCODE_F6: return EKeys::F6;
			case SCANCODE_F7: return EKeys::F7;
			case SCANCODE_F8: return EKeys::F8;
			case SCANCODE_F9: return EKeys::F9;
			case SCANCODE_F10: return EKeys::F10;
			case SCANCODE_F11: return EKeys::F11;
			case SCANCODE_F12: return EKeys::F12;
			case SCANCODE_HOME: return EKeys::Home;
			case SCANCODE_INSERT: return EKeys::Insert;
			case SCANCODE_LEFT: return EKeys::Left;
			case SCANCODE_LEFTALT: return EKeys::LeftAlt;
			case SCANCODE_LEFTCTRL: return EKeys::LeftControl;
			case SCANCODE_LEFTMETA: return EKeys::LeftCommand;
			case SCANCODE_LEFTSHIFT: return EKeys::LeftShift;
			case SCANCODE_PAGEDOWN: return EKeys::PageDown;
			case SCANCODE_PAGEUP: return EKeys::PageUp;
			case SCANCODE_RIGHT: return EKeys::Right;
			case SCANCODE_RIGHTALT: return EKeys::RightAlt;
			case SCANCODE_RIGHTCTRL: return EKeys::RightControl;
			case SCANCODE_RIGHTMETA: return EKeys::RightCommand;
			case SCANCODE_RIGHTSHIFT: return EKeys::RightShift;
			case SCANCODE_SPACE: return EKeys::SpaceBar;
			case SCANCODE_TAB: return EKeys::Tab;
			case SCANCODE_UP: return EKeys::Up;
			case SCANCODE_NUMLOCK: return EKeys::NumLock;
			case SCANCODE_KPSLASH: return EKeys::Divide;
			case SCANCODE_KPASTERISK: return EKeys::Multiply;
			case SCANCODE_KPMINUS: return EKeys::Subtract;
			case SCANCODE_KPPLUS: return EKeys::Add;
			case SCANCODE_KPENTER: return EKeys::Enter;
			case SCANCODE_KP1: return EKeys::NumPadOne;
			case SCANCODE_KP2: return EKeys::NumPadTwo;
			case SCANCODE_KP3: return EKeys::NumPadThree;
			case SCANCODE_KP4: return EKeys::NumPadFour;
			case SCANCODE_KP5: return EKeys::NumPadFive;
			case SCANCODE_KP6: return EKeys::NumPadSix;
			case SCANCODE_KP7: return EKeys::NumPadSeven;
			case SCANCODE_KP8: return EKeys::NumPadEight;
			case SCANCODE_KP9: return EKeys::NumPadNine;
			case SCANCODE_KP0: return EKeys::NumPadZero;
			case SCANCODE_KPDOT: return EKeys::Decimal;
			case SCANCODE_KPEQUAL: return EKeys::Equals;
			default: return EKeys::Invalid;
		}
	}
	if(Source == VirtualKey)
	{
		switch(KeyCode)
		{
		case VK_KEY_0: return EKeys::Zero;
		case VK_KEY_1: return EKeys::One;
		case VK_KEY_2: return EKeys::Two;
		case VK_KEY_3: return EKeys::Three;
		case VK_KEY_4: return EKeys::Four;
		case VK_KEY_5: return EKeys::Five;
		case VK_KEY_6: return EKeys::Six;
		case VK_KEY_7: return EKeys::Seven;
		case VK_KEY_8: return EKeys::Eight;
		case VK_KEY_9: return EKeys::Nine;
		case VK_KEY_A: return EKeys::A;
		case VK_KEY_B: return EKeys::B;
		case VK_KEY_C: return EKeys::C;
		case VK_KEY_D: return EKeys::D;
		case VK_KEY_E: return EKeys::E;
		case VK_KEY_F: return EKeys::F;
		case VK_KEY_G: return EKeys::G;
		case VK_KEY_H: return EKeys::H;
		case VK_KEY_I: return EKeys::I;
		case VK_KEY_J: return EKeys::J;
		case VK_KEY_K: return EKeys::K;
		case VK_KEY_L: return EKeys::L;
		case VK_KEY_M: return EKeys::M;
		case VK_KEY_N: return EKeys::N;
		case VK_KEY_O: return EKeys::O;
		case VK_KEY_P: return EKeys::P;
		case VK_KEY_Q: return EKeys::Q;
		case VK_KEY_R: return EKeys::R;
		case VK_KEY_S: return EKeys::S;
		case VK_KEY_T: return EKeys::T;
		case VK_KEY_U: return EKeys::U;
		case VK_KEY_V: return EKeys::V;
		case VK_KEY_W: return EKeys::W;
		case VK_KEY_X: return EKeys::X;
		case VK_KEY_Y: return EKeys::Y;
		case VK_KEY_Z: return EKeys::Z;
		case VK_OEM_1: return EKeys::Semicolon;
		case VK_OEM_2: return EKeys::Slash;
		case VK_OEM_3: return EKeys::Tilde;
		case VK_OEM_4: return EKeys::LeftBracket;
		case VK_OEM_5: return EKeys::Backslash;
		case VK_OEM_6: return EKeys::RightBracket;
		case VK_OEM_7: return EKeys::Apostrophe;
		case VK_OEM_COMMA: return EKeys::Comma;
		case VK_OEM_PLUS: return EKeys::Equals;
		case VK_OEM_MINUS: return EKeys::Hyphen;
		case VK_OEM_PERIOD: return EKeys::Period;
		case VK_BACK: return EKeys::BackSpace;
		case VK_CAPITAL: return EKeys::CapsLock;
		case VK_DELETE: return EKeys::Delete;
		case VK_DOWN: return EKeys::Down;
		case VK_END: return EKeys::End;
		case VK_ESCAPE: return EKeys::Escape;
		case VK_F1: return EKeys::F1;
		case VK_F2: return EKeys::F2;
		case VK_F3: return EKeys::F3;
		case VK_F4: return EKeys::F4;
		case VK_F5: return EKeys::F5;
		case VK_F6: return EKeys::F6;
		case VK_F7: return EKeys::F7;
		case VK_F8: return EKeys::F8;
		case VK_F9: return EKeys::F9;
		case VK_F10: return EKeys::F10;
		case VK_F11: return EKeys::F11;
		case VK_F12: return EKeys::F12;
		case VK_HOME: return EKeys::Home;
		case VK_INSERT: return EKeys::Insert;
		case VK_LEFT: return EKeys::Left;
		case VK_LMENU: return EKeys::LeftAlt;
		case VK_LCONTROL: return EKeys::LeftControl;
		case VK_LWIN: return EKeys::LeftCommand;
		case VK_LSHIFT: return EKeys::LeftShift;
		case VK_NEXT: return EKeys::PageDown;
		case VK_PRIOR: return EKeys::PageUp;
		case VK_RIGHT: return EKeys::Right;
		case VK_RMENU: return EKeys::RightAlt;
		case VK_RCONTROL: return EKeys::RightControl;
		case VK_RWIN: return EKeys::RightCommand;
		case VK_RSHIFT: return EKeys::RightShift;
		case VK_SPACE: return EKeys::SpaceBar;
		case VK_TAB: return EKeys::Tab;
		case VK_UP: return EKeys::Up;
		case VK_NUMLOCK: return EKeys::NumLock;
		case VK_DIVIDE: return EKeys::Divide;
		case VK_MULTIPLY: return EKeys::Multiply;
		case VK_SUBTRACT: return EKeys::Subtract;
		case VK_ADD: return EKeys::Add;
		case VK_RETURN: return EKeys::Enter;
		case VK_NUMPAD1: return EKeys::NumPadOne;
		case VK_NUMPAD2: return EKeys::NumPadTwo;
		case VK_NUMPAD3: return EKeys::NumPadThree;
		case VK_NUMPAD4: return EKeys::NumPadFour;
		case VK_NUMPAD5: return EKeys::NumPadFive;
		case VK_NUMPAD6: return EKeys::NumPadSix;
		case VK_NUMPAD7: return EKeys::NumPadSeven;
		case VK_NUMPAD8: return EKeys::NumPadEight;
		case VK_NUMPAD9: return EKeys::NumPadNine;
		case VK_NUMPAD0: return EKeys::NumPadZero;
		case VK_DECIMAL: return EKeys::Decimal;
		default: return EKeys::Invalid;
		}
	}

	return FInputKeyManager::Get().GetKeyFromCodes(KeyCode, CharCode);
}