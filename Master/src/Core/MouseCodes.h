#pragma once

#include <stdint.h>
#include <map>

namespace Puppeteer
{
	using MouseCode = uint16_t;

	namespace Mouse
	{
		typedef enum : MouseCode
		{
			// From glfw3.h
			Button0 = 0,
			Button1 = 1,
			Button2 = 2,
			Button3 = 3,
			Button4 = 4,
			Button5 = 5,
			Button6 = 6,
			Button7 = 7,

			ButtonLast = Button7,
			ButtonLeft = Button0,
			ButtonRight = Button1,
			ButtonMiddle = Button2
		} MouseCodes;
		static std::map<int, int> WindowsMouseDown =
		{
			{MouseCodes::ButtonLeft, 0x0002},
			{MouseCodes::ButtonRight,	0x0008},
			{MouseCodes::ButtonMiddle, 0x0020},
			{MouseCodes::ButtonLast, 0x0080},
		};

		static std::map<int, int> WindowsMouseUp =
		{
			{MouseCodes::ButtonLeft, 0x0004},
			{MouseCodes::ButtonRight,	0x0010},
			{MouseCodes::ButtonMiddle, 0x0040},
			{MouseCodes::ButtonLast, 0x00100},
		};
	}
}