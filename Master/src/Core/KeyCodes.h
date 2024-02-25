#pragma once
#include <map>
#include <stdint.h>

namespace Puppeteer
{
	using KeyCode = uint16_t;

	namespace Key
	{
		typedef enum : KeyCode
		{
			// From glfw3.h
			Space = 32,
			Apostrophe = 39, /* ' */
			Comma = 44, /* , */
			Minus = 45, /* - */
			Period = 46, /* . */
			Slash = 47, /* / */

			D0 = 48, /* 0 */
			D1 = 49, /* 1 */
			D2 = 50, /* 2 */
			D3 = 51, /* 3 */
			D4 = 52, /* 4 */
			D5 = 53, /* 5 */
			D6 = 54, /* 6 */
			D7 = 55, /* 7 */
			D8 = 56, /* 8 */
			D9 = 57, /* 9 */

			Semicolon = 59, /* ; */
			Equal = 61, /* = */

			A = 65,
			B = 66,
			C = 67,
			D = 68,
			E = 69,
			F = 70,
			G = 71,
			H = 72,
			I = 73,
			J = 74,
			K = 75,
			L = 76,
			M = 77,
			N = 78,
			O = 79,
			P = 80,
			Q = 81,
			R = 82,
			S = 83,
			T = 84,
			U = 85,
			V = 86,
			W = 87,
			X = 88,
			Y = 89,
			Z = 90,

			LeftBracket = 91,  /* [ */
			Backslash = 92,  /* \ */
			RightBracket = 93,  /* ] */
			GraveAccent = 96,  /* ` */

			World1 = 161, /* non-US #1 */
			World2 = 162, /* non-US #2 */

			/* Function keys */
			Escape = 256,
			Enter = 257,
			Tab = 258,
			Backspace = 259,
			Insert = 260,
			Delete = 261,
			Right = 262,
			Left = 263,
			Down = 264,
			Up = 265,
			PageUp = 266,
			PageDown = 267,
			Home = 268,
			End = 269,
			CapsLock = 280,
			ScrollLock = 281,
			NumLock = 282,
			PrintScreen = 283,
			Pause = 284,
			F1 = 290,
			F2 = 291,
			F3 = 292,
			F4 = 293,
			F5 = 294,
			F6 = 295,
			F7 = 296,
			F8 = 297,
			F9 = 298,
			F10 = 299,
			F11 = 300,
			F12 = 301,
			F13 = 302,
			F14 = 303,
			F15 = 304,
			F16 = 305,
			F17 = 306,
			F18 = 307,
			F19 = 308,
			F20 = 309,
			F21 = 310,
			F22 = 311,
			F23 = 312,
			F24 = 313,
			F25 = 314,

			/* Keypad */
			KP0 = 320,
			KP1 = 321,
			KP2 = 322,
			KP3 = 323,
			KP4 = 324,
			KP5 = 325,
			KP6 = 326,
			KP7 = 327,
			KP8 = 328,
			KP9 = 329,
			KPDecimal = 330,
			KPDivide = 331,
			KPMultiply = 332,
			KPSubtract = 333,
			KPAdd = 334,
			KPEnter = 335,
			KPEqual = 336,

			LeftShift = 340,
			LeftControl = 341,
			LeftAlt = 342,
			LeftSuper = 343,
			RightShift = 344,
			RightControl = 345,
			RightAlt = 346,
			RightSuper = 347,
			Menu = 348
		} KeyCodes;

		static std::map<int, int> WindowsCodes = {
			{KeyCodes::Backspace, 0x08},
			{KeyCodes::Tab, 0x09},
			{KeyCodes::Enter, 0x0D},
			{KeyCodes::LeftShift, 0xA0},
			{KeyCodes::RightShift, 0xA1},
			{KeyCodes::LeftControl, 0xA2},
			{KeyCodes::RightControl, 0xA3},
			{KeyCodes::LeftAlt, 0xA4},
			{KeyCodes::RightAlt, 0xA5},
			{KeyCodes::Pause, 0x13},
			{KeyCodes::CapsLock, 0x14},
			{KeyCodes::Escape, 0x1B},
			{KeyCodes::Space, 0x20},
			{KeyCodes::PageUp, 0x21},
			{KeyCodes::PageDown, 0x22},
			{KeyCodes::End, 0x23},
			{KeyCodes::Home, 0x24},
			{KeyCodes::Left, 0x25},
			{KeyCodes::Up, 0x26},
			{KeyCodes::Right, 0x27},
			{KeyCodes::Down, 0x28},
			{KeyCodes::PrintScreen, 0x2C},
			{KeyCodes::Insert, 0x2D},
			{KeyCodes::Delete, 0x2E},
			{KeyCodes::D0, 0x30},
			{KeyCodes::D1, 0x31},
			{KeyCodes::D2, 0x32},
			{KeyCodes::D3, 0x33},
			{KeyCodes::D4, 0x34},
			{KeyCodes::D5, 0x35},
			{KeyCodes::D6, 0x36},
			{KeyCodes::D7, 0x37},
			{KeyCodes::D8, 0x38},
			{KeyCodes::D9, 0x39},
			{KeyCodes::A, 0x41},
			{KeyCodes::B, 0x42},
			{KeyCodes::C, 0x43},
			{KeyCodes::D, 0x44},
			{KeyCodes::E, 0x45},
			{KeyCodes::F, 0x46},
			{KeyCodes::G, 0x47},
			{KeyCodes::H, 0x48},
			{KeyCodes::I, 0x49},
			{KeyCodes::J, 0x4A},
			{KeyCodes::K, 0x4B},
			{KeyCodes::L, 0x4C},
			{KeyCodes::M, 0x4D},
			{KeyCodes::N, 0x4E},
			{KeyCodes::O, 0x4F},
			{KeyCodes::P, 0x50},
			{KeyCodes::Q, 0x51},
			{KeyCodes::R, 0x52},
			{KeyCodes::S, 0x53},
			{KeyCodes::T, 0x54},
			{KeyCodes::U, 0x55},
			{KeyCodes::V, 0x56},
			{KeyCodes::W, 0x57},
			{KeyCodes::X, 0x58},
			{KeyCodes::Y, 0x59},
			{KeyCodes::Z, 0x5A},
			{KeyCodes::LeftSuper, 0x5B},
			{KeyCodes::RightSuper, 0x5C},
			{KeyCodes::Menu, 0x5D},
			{KeyCodes::KP0, 0x60},
			{KeyCodes::KP1, 0x61},
			{KeyCodes::KP2, 0x62},
			{KeyCodes::KP3, 0x63},
			{KeyCodes::KP4, 0x64},
			{KeyCodes::KP5, 0x65},
			{KeyCodes::KP6, 0x66},
			{KeyCodes::KP7, 0x67},
			{KeyCodes::KP8, 0x68},
			{KeyCodes::KP9, 0x69},
			{KeyCodes::KPMultiply, 0x6A},
			{KeyCodes::KPAdd, 0x6B},
			//{KeyCodes::KPDivide, 0x6C},
			{KeyCodes::KPSubtract, 0x6D},
			{KeyCodes::KPDecimal, 0x6E},
			{KeyCodes::KPDivide, 0x6F},
			{KeyCodes::F1, 0x70},
			{KeyCodes::F2, 0x71},
			{KeyCodes::F3, 0x72},
			{KeyCodes::F4, 0x73},
			{KeyCodes::F5, 0x74},
			{KeyCodes::F6, 0x75},
			{KeyCodes::F7, 0x76},
			{KeyCodes::F8, 0x77},
			{KeyCodes::F9, 0x78},
			{KeyCodes::F10, 0x79},
			{KeyCodes::F11, 0x7A},
			{KeyCodes::F12, 0x7B},
			{KeyCodes::F13, 0x7C},
			{KeyCodes::F14, 0x7D},
			{KeyCodes::F15, 0x7E},
			{KeyCodes::F16, 0x7F},
			{KeyCodes::F17, 0x80},
			{KeyCodes::F18, 0x81},
			{KeyCodes::F19, 0x82},
			{KeyCodes::F20, 0x83},
			{KeyCodes::F21, 0x84},
			{KeyCodes::F22, 0x85},
			{KeyCodes::F23, 0x86},
			{KeyCodes::F24, 0x87},
			{KeyCodes::NumLock, 0x90},
			{KeyCodes::ScrollLock, 0x91},
			{KeyCodes::LeftBracket, 0xDB},
			{KeyCodes::Backslash, 0xDC},
			{KeyCodes::RightBracket, 0xDD},
			{KeyCodes::GraveAccent, 0xC0},
			{KeyCodes::World1, 0xE1},
			{KeyCodes::World2, 0xE2},
			{KeyCodes::Apostrophe, 0xDE},
			{KeyCodes::Comma, 0xBC},
			{KeyCodes::Minus, 0xBD},
			{KeyCodes::Period, 0xBE},
			{KeyCodes::Slash, 0xBF},
			{KeyCodes::Semicolon, 0xBA},
			{KeyCodes::KPEnter, 0x0D},
		};
	}
}