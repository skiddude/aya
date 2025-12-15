//
//  KeyCode.cpp
//  App
//
//  Created by Ben Tkacheff on 11/19/13.
//
//


#include "Utility/KeyCode.hpp"
#include "Reflection/EnumConverter.hpp"

namespace Aya
{

namespace Reflection
{

template<>
EnumDesc<KeyCode>::EnumDesc()
    : EnumDescriptor("KeyCode")
{
    addPair(AYA_SDLK_UNKNOWN, "Unknown");
    addPair(AYA_SDLK_BACKSPACE, "Backspace");
    addPair(AYA_SDLK_TAB, "Tab");
    addPair(AYA_SDLK_CLEAR, "Clear");
    addPair(AYA_SDLK_RETURN, "Return");
    addPair(AYA_SDLK_PAUSE, "Pause");
    addPair(AYA_SDLK_ESCAPE, "Escape");
    addPair(AYA_SDLK_SPACE, "Space");
    addPair(AYA_SDLK_QUOTEDBL, "QuotedDouble");
    addPair(AYA_SDLK_HASH, "Hash");
    addPair(AYA_SDLK_DOLLAR, "Dollar");
    addPair(AYA_SDLK_PERCENT, "Percent");
    addPair(AYA_SDLK_AMPERSAND, "Ampersand");
    addPair(AYA_SDLK_QUOTE, "Quote");
    addPair(AYA_SDLK_LEFTPAREN, "LeftParenthesis");
    addPair(AYA_SDLK_RIGHTPAREN, "RightParenthesis");
    addPair(AYA_SDLK_ASTERISK, "Asterisk");
    addPair(AYA_SDLK_PLUS, "Plus");
    addPair(AYA_SDLK_COMMA, "Comma");
    addPair(AYA_SDLK_MINUS, "Minus");
    addPair(AYA_SDLK_PERIOD, "Period");
    addPair(AYA_SDLK_SLASH, "Slash");

    addPair(AYA_SDLK_0, "Zero");
    addPair(AYA_SDLK_1, "One");
    addPair(AYA_SDLK_2, "Two");
    addPair(AYA_SDLK_3, "Three");
    addPair(AYA_SDLK_4, "Four");
    addPair(AYA_SDLK_5, "Five");
    addPair(AYA_SDLK_6, "Six");
    addPair(AYA_SDLK_7, "Seven");
    addPair(AYA_SDLK_8, "Eight");
    addPair(AYA_SDLK_9, "Nine");

    addPair(AYA_SDLK_COLON, "Colon");
    addPair(AYA_SDLK_SEMICOLON, "Semicolon");
    addPair(AYA_SDLK_LESS, "LessThan");
    addPair(AYA_SDLK_EQUALS, "Equals");
    addPair(AYA_SDLK_GREATER, "GreaterThan");
    addPair(AYA_SDLK_QUESTION, "Question");
    addPair(AYA_SDLK_AT, "At");
    addPair(AYA_SDLK_LEFTBRACKET, "LeftBracket");
    addPair(AYA_SDLK_BACKSLASH, "BackSlash");
    addPair(AYA_SDLK_RIGHTBRACKET, "RightBracket");
    addPair(AYA_SDLK_CARET, "Caret");
    addPair(AYA_SDLK_UNDERSCORE, "Underscore");
    addPair(AYA_SDLK_BACKQUOTE, "Backquote");

    addPair(AYA_SDLK_a, "A");
    addPair(AYA_SDLK_b, "B");
    addPair(AYA_SDLK_c, "C");
    addPair(AYA_SDLK_d, "D");
    addPair(AYA_SDLK_e, "E");
    addPair(AYA_SDLK_f, "F");
    addPair(AYA_SDLK_g, "G");
    addPair(AYA_SDLK_h, "H");
    addPair(AYA_SDLK_i, "I");
    addPair(AYA_SDLK_j, "J");
    addPair(AYA_SDLK_k, "K");
    addPair(AYA_SDLK_l, "L");
    addPair(AYA_SDLK_m, "M");
    addPair(AYA_SDLK_n, "N");
    addPair(AYA_SDLK_o, "O");
    addPair(AYA_SDLK_p, "P");
    addPair(AYA_SDLK_q, "Q");
    addPair(AYA_SDLK_r, "R");
    addPair(AYA_SDLK_s, "S");
    addPair(AYA_SDLK_t, "T");
    addPair(AYA_SDLK_u, "U");
    addPair(AYA_SDLK_v, "V");
    addPair(AYA_SDLK_w, "W");
    addPair(AYA_SDLK_x, "X");
    addPair(AYA_SDLK_y, "Y");
    addPair(AYA_SDLK_z, "Z");

    addPair(AYA_SDLK_LEFTCURLY, "LeftCurly");
    addPair(AYA_SDLK_PIPE, "Pipe");
    addPair(AYA_SDLK_RIGHTCURLY, "RightCurly");
    addPair(AYA_SDLK_TILDE, "Tilde");
    addPair(AYA_SDLK_DELETE, "Delete");

    addPair(AYA_SDLK_KP0, "KeypadZero");
    addPair(AYA_SDLK_KP1, "KeypadOne");
    addPair(AYA_SDLK_KP2, "KeypadTwo");
    addPair(AYA_SDLK_KP3, "KeypadThree");
    addPair(AYA_SDLK_KP4, "KeypadFour");
    addPair(AYA_SDLK_KP5, "KeypadFive");
    addPair(AYA_SDLK_KP6, "KeypadSix");
    addPair(AYA_SDLK_KP7, "KeypadSeven");
    addPair(AYA_SDLK_KP8, "KeypadEight");
    addPair(AYA_SDLK_KP9, "KeypadNine");
    addPair(AYA_SDLK_KP_PERIOD, "KeypadPeriod");
    addPair(AYA_SDLK_KP_DIVIDE, "KeypadDivide");
    addPair(AYA_SDLK_KP_MULTIPLY, "KeypadMultiply");
    addPair(AYA_SDLK_KP_MINUS, "KeypadMinus");
    addPair(AYA_SDLK_KP_PLUS, "KeypadPlus");
    addPair(AYA_SDLK_KP_ENTER, "KeypadEnter");
    addPair(AYA_SDLK_KP_EQUALS, "KeypadEquals");

    addPair(AYA_SDLK_UP, "Up");
    addPair(AYA_SDLK_DOWN, "Down");
    addPair(AYA_SDLK_RIGHT, "Right");
    addPair(AYA_SDLK_LEFT, "Left");
    addPair(AYA_SDLK_INSERT, "Insert");
    addPair(AYA_SDLK_HOME, "Home");
    addPair(AYA_SDLK_END, "End");
    addPair(AYA_SDLK_PAGEUP, "PageUp");
    addPair(AYA_SDLK_PAGEDOWN, "PageDown");

    addPair(AYA_SDLK_LSHIFT, "LeftShift");
    addPair(AYA_SDLK_RSHIFT, "RightShift");
    addPair(AYA_SDLK_LMETA, "LeftMeta");
    addPair(AYA_SDLK_RMETA, "RightMeta");
    addPair(AYA_SDLK_LALT, "LeftAlt");
    addPair(AYA_SDLK_RALT, "RightAlt");
    addPair(AYA_SDLK_LCTRL, "LeftControl");
    addPair(AYA_SDLK_RCTRL, "RightControl");
    addPair(AYA_SDLK_CAPSLOCK, "CapsLock");
    addPair(AYA_SDLK_NUMLOCK, "NumLock");
    addPair(AYA_SDLK_SCROLLOCK, "ScrollLock");

    addPair(AYA_SDLK_LSUPER, "LeftSuper");
    addPair(AYA_SDLK_RSUPER, "RightSuper");
    addPair(AYA_SDLK_MODE, "Mode");
    addPair(AYA_SDLK_COMPOSE, "Compose");

    addPair(AYA_SDLK_HELP, "Help");
    addPair(AYA_SDLK_PRINT, "Print");
    addPair(AYA_SDLK_SYSREQ, "SysReq");
    addPair(AYA_SDLK_BREAK, "Break");
    addPair(AYA_SDLK_MENU, "Menu");
    addPair(AYA_SDLK_POWER, "Power");
    addPair(AYA_SDLK_EURO, "Euro");
    addPair(AYA_SDLK_UNDO, "Undo");

    addPair(AYA_SDLK_F1, "F1");
    addPair(AYA_SDLK_F2, "F2");
    addPair(AYA_SDLK_F3, "F3");
    addPair(AYA_SDLK_F4, "F4");
    addPair(AYA_SDLK_F5, "F5");
    addPair(AYA_SDLK_F6, "F6");
    addPair(AYA_SDLK_F7, "F7");
    addPair(AYA_SDLK_F8, "F8");
    addPair(AYA_SDLK_F9, "F9");
    addPair(AYA_SDLK_F10, "F10");
    addPair(AYA_SDLK_F11, "F11");
    addPair(AYA_SDLK_F12, "F12");
    addPair(AYA_SDLK_F13, "F13");
    addPair(AYA_SDLK_F14, "F14");
    addPair(AYA_SDLK_F15, "F15");

    const std::string worldString("World");
    for (int i = AYA_SDLK_WORLD_0; i <= AYA_SDLK_WORLD_95; ++i)
    {
        int num = i - AYA_SDLK_WORLD_0;

        std::stringstream ss;
        ss << worldString;
        ss << num;

        addPair((Aya::KeyCode)i, ss.str().c_str());
    }

    addPair(AYA_SDLK_GAMEPAD_BUTTONX, "ButtonX");
    addPair(AYA_SDLK_GAMEPAD_BUTTONY, "ButtonY");
    addPair(AYA_SDLK_GAMEPAD_BUTTONA, "ButtonA");
    addPair(AYA_SDLK_GAMEPAD_BUTTONB, "ButtonB");
    addPair(AYA_SDLK_GAMEPAD_BUTTONR1, "ButtonR1");
    addPair(AYA_SDLK_GAMEPAD_BUTTONL1, "ButtonL1");
    addPair(AYA_SDLK_GAMEPAD_BUTTONR2, "ButtonR2");
    addPair(AYA_SDLK_GAMEPAD_BUTTONL2, "ButtonL2");
    addPair(AYA_SDLK_GAMEPAD_BUTTONR3, "ButtonR3");
    addPair(AYA_SDLK_GAMEPAD_BUTTONL3, "ButtonL3");
    addPair(AYA_SDLK_GAMEPAD_BUTTONSTART, "ButtonStart");
    addPair(AYA_SDLK_GAMEPAD_BUTTONSELECT, "ButtonSelect");
    addPair(AYA_SDLK_GAMEPAD_DPADLEFT, "DPadLeft");
    addPair(AYA_SDLK_GAMEPAD_DPADRIGHT, "DPadRight");
    addPair(AYA_SDLK_GAMEPAD_DPADUP, "DPadUp");
    addPair(AYA_SDLK_GAMEPAD_DPADDOWN, "DPadDown");
    addPair(AYA_SDLK_GAMEPAD_THUMBSTICK1, "Thumbstick1");
    addPair(AYA_SDLK_GAMEPAD_THUMBSTICK2, "Thumbstick2");
}

template<>
KeyCode& Variant::convert<Aya::KeyCode>(void)
{
    return genericConvert<Aya::KeyCode>();
}

} // namespace Reflection

template<>
bool StringConverter<Aya::KeyCode>::convertToValue(const std::string& text, Aya::KeyCode& value)
{
    return Reflection::EnumDesc<Aya::KeyCode>::singleton().convertToValue(text.c_str(), value);
}

} // namespace Aya