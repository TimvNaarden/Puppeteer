#pragma once

#include <Windows.h>

class Screencap
{
public:
	Screencap(int width, int height);
	~Screencap();

	void Resize(int width, int height);

	unsigned char* CaptureScreen(int x, int y);

	inline size_t GetBufferSize() const { return m_BufferSize; }
	inline int GetWidth() const { return m_Width; }
	inline int GetHeight() const { return m_Height; }
	inline int GetSize() const { return m_Width * m_Height * 4; }
private:
	int m_Width, m_Height;
	unsigned char* m_Buffer = nullptr;
	size_t m_BufferSize;

	HDC m_hScreenDC; // device context for the entire screen
	HDC m_hCaptureDC; // compatible device context for the capture
	HBITMAP m_hBitmap; // bitmap to store the captured image

	BITMAPINFOHEADER m_BI;
};