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
// Copyright (c) 2020-2024 Tristan Grimmer.
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
#include <Image/tFrame.h>
#include <Image/tBaseImage.h>
namespace tImage
{


class tImageICO : public tBaseImage
{
public:
	// Creates an invalid tImageICO. You must call Load manually.
	tImageICO()																											{ }
	tImageICO(const tString& icoFile)																					{ Load(icoFile); }

	// Creates a tImageICO from a bunch of frames. If steal is true, the srcFrames will be empty after.
	tImageICO(tList<tFrame>& srcFrames, bool stealFrames)																{ Set(srcFrames, stealFrames); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImageICO(tPixel4b* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageICO(tFrame* frame, bool steal = true)																			{ Set(frame, steal); }

	// Constructs from a tPicture. Single-frame.
	tImageICO(tPicture& picture, bool steal = true)																		{ Set(picture, steal); }

	virtual ~tImageICO()																								{ Clear(); }

	// Clears the current tImageICO before loading. If false returned object is invalid.
	bool Load(const tString& icoFile);
	bool Load(const uint8* icoFileInMemory, int numBytes);

	// This one sets from a supplied pixel array.
	bool Set(tList<tFrame>& srcFrames, bool stealFrames);

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel4b* pixels, int width, int height, bool steal = false) override;

	// Sets from a single frame.
	bool Set(tFrame*, bool steal = true) override;

	// Sets from a tPicture.
	bool Set(tPicture& picture, bool steal = true) override;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return (GetNumFrames() >= 1); }
	int GetNumFrames() const																							{ return Frames.GetNumItems(); }

	// After this call you are the owner of the frame and must eventually delete it. The frame you stole will no longer
	// be part of the tImageICO, but the remaining ones will still be there. GetNumFrames will be one fewer.
	tFrame* StealFrame(int frameNum);
	tFrame* GetFrame(bool steal = true) override;

	// Similar to above but takes all the frames from the tImageICO and appends them to the supplied frame list. The
	// object will be invalid after since it will have no frames.
	void StealFrames(tList<tFrame>&);

	// Returns a pointer to the frame, but it's not yours to delete. This object still owns it.
	tFrame* GetFrame(int frameNum);

private:
	// Different frames of an ICO file may have different pixel formats. This function uses bpp as the metric to find
	// the 'best' one used in all frames. It uses this function to set PixelFormatSrc.
	tPixelFormat GetBestSrcPixelFormat() const;

	tFrame* CreateFrame(const uint8* buffer, int width, int height, int numBytes);

	tList<tFrame> Frames;
};


// Implementation only below.


inline tFrame* tImage::tImageICO::StealFrame(int frameNum)
{
	tFrame* p = GetFrame(frameNum);
	if (!p)
		return nullptr;

	return Frames.Remove(p);
}


inline void tImage::tImageICO::StealFrames(tList<tFrame>& frames)
{
	while (tFrame* frame = Frames.Remove())
		frames.Append(frame);
}


inline tFrame* tImage::tImageICO::GetFrame(int frameNum)
{
	if ((frameNum >= Frames.GetNumItems()) || (frameNum < 0))
		return nullptr;

	tFrame* f = Frames.First();
	while (frameNum--)
		f = f->Next();

	return f;
}


inline void tImageICO::Clear()
{
	while (tFrame* frame = Frames.Remove())
		delete frame;

	tBaseImage::Clear();
}


inline tPixelFormat tImageICO::GetBestSrcPixelFormat() const
{
	tPixelFormat bestFormat = tPixelFormat::Invalid;
	for (tFrame* frame = Frames.First(); frame; frame = frame->Next())
	{
		if (frame->PixelFormatSrc == tPixelFormat::Invalid)
			continue;

		// Early exit as can't do better than 32-bit for an ico file.
		if (frame->PixelFormatSrc == tPixelFormat::R8G8B8A8)
			return tPixelFormat::R8G8B8A8;

		// Otherwise use the bpp metric to determine the 'best'.
		else if (tGetBitsPerPixelFloat(frame->PixelFormatSrc) > tGetBitsPerPixelFloat(bestFormat))
			bestFormat = frame->PixelFormatSrc;
	}
	
	return bestFormat;
}


}
