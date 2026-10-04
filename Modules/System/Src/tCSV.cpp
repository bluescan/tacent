// tCSV.cpp
//
// Implementation of tCSV's file-loading and string-parsing entry points. The read-only accessor methods
// (GetNumColumns, Get, GetRow, GetColumn) and the tCSVRow helpers are implemented inline in tCSV.h, below the
// "Implementation only below this line." marker.
//
// Copyright (c) 2026 Tristan Grimmer.
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
// Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <Foundation/tFundamentals.h>
#include <System/tCSV.h>
#include <System/tFile.h>
namespace tSystem
{


bool tCSV::LoadFile(const tString& file)
{
	tString text;
	if (!tLoadFile(file, text))
	{
		// Could not open the file: discard any previous content and leave the document empty.
		Rows.Empty();
		return false;
	}

	LoadString(text);
	return true;
}


void tCSV::LoadString(const tString& text)
{
	// Start from a clean slate.
	Rows.Empty();

	// Normalise line endings so that only '\n' separates rows. Handles Windows (CRLF), old-Mac (CR) and Unix (LF).
	tString normalised(text);
	normalised.Replace("\r\n", "\n");
	normalised.Replace("\r", "\n");

	// First pass: split the normalised text into the raw row strings, skipping blank rows.
	tList<tStringItem> rowStrings;
	tStd::tExplode(rowStrings, normalised, '\n');

	// Second pass: split each non-blank row into quote-aware columns and store the resulting row. The columns are
	// created and appended into the (owning) row's column list, so they are freed along with the row.
	for (tStringItem* rowString = rowStrings.First(); rowString; rowString = rowString->Next())
	{
		if (rowString->IsEmpty())
			continue;

		tCSVRow* row = new tCSVRow();
		tStd::tExplode(row->Columns, *rowString, ',', '"');
		Rows.Append(row);
	}
}


}
