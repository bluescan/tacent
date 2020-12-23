// tImageICO.h
//
// This class knows how to load windows icon (ico) files. It loads the data into multiple tPixel arrays, one for each
// frame (ico files may be multiple images at different resolutions). These arrays may be 'stolen' by tPictures. The
// loading code is a modificaton of code from Victor Laskin. In particular the code now:
// a) Loads all frames of an ico, not just the biggest one.
// b) Supports embedded png images.
// c) Supports widths and heights of 256.
// Victor Laskin's header/licence in the original ico.cpp is shown below.
//
// Copyright (c) 2020 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.
//
// Includes modified version of code from Victor Laskin.
// Code by Victor Laskin (victor.laskin@gmail.com)
// Rev 2 - 1bit color was added, fixes for bit mask.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once
#include <Foundation/tString.h>
#include <Math/tColour.h>
#include <Image/tPixelFormat.h>
namespace tImage
{


class tImageICO
{
public:
	// Creates an invalid tImageICO. You must call Load manually.
	tImageICO()																											{ }
	tImageICO(const tString& icoFile)																					{ Load(icoFile); }

	virtual ~tImageICO()																								{ Clear(); }

	// Clears the current tImageICO before loading. If false returned object is invalid.
	bool Load(const tString& icoFile);

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear();
	bool IsValid() const																								{ return (GetNumFrames() >= 1); }
	int GetNumFrames() const																							{ return Frames.GetNumItems(); }
	tPixelFormat GetBestSrcPixelFormat() const;

	struct Frame : public tLink<Frame>
	{
		int Width					= 0;
		int Height					= 0;
		tPixel* Pixels				= nullptr;
		tPixelFormat SrcPixelFormat	= tPixelFormat::Invalid;
	};

	// After this call you are the owner of the frame and must eventually delete it. The frame you stole will no
	// longer be a valid frame of the tImageICO, but the remaining ones will still be valid.
	Frame* StealFrame(int frameNum);
	Frame* GetFrame(int frameNum);

private:
	bool PopulateFrames(const uint8* buffer, int numBytes);	
	Frame* CreateFrame(const uint8* buffer, int width, int height, int numBytes);

	tList<Frame> Frames;
};


// Implementation only below.


inline tImageICO::Frame* tImage::tImageICO::StealFrame(int frameNum)
{
	Frame* p = GetFrame(frameNum);
	if (!p)
		return nullptr;

	return Frames.Remove(p);
}


inline tImageICO::Frame* tImage::tImageICO::GetFrame(int frameNum)
{
	if ((frameNum >= Frames.GetNumItems()) || (frameNum < 0))
		return nullptr;

	Frame* f = Frames.First();
	while (frameNum--)
		f = f->Next();

	return f;
}


inline void tImageICO::Clear()
{
	while (Frame* frame = Frames.Remove())
	{
		delete[] frame->Pixels;
		delete frame;
	}
}


inline tPixelFormat tImageICO::GetBestSrcPixelFormat() const
{
	tPixelFormat bestFormat = tPixelFormat::Invalid;
	for (Frame* frame = Frames.First(); frame; frame = frame->Next())
	{
		if (frame->SrcPixelFormat == tPixelFormat::Invalid)
			continue;
		if (frame->SrcPixelFormat == tPixelFormat::R8G8B8A8)
			return tPixelFormat::R8G8B8A8;
		else if (frame->SrcPixelFormat < bestFormat)
			bestFormat = frame->SrcPixelFormat;
	}
	
	return bestFormat;
}


}
