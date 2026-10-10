// tCSV.cpp
//
// Implementation of tCSV's file-loading, string-parsing, mutation and file-saving entry points. The read accessors
// (GetNumColumns, Get, GetRow, GetColumn) and the tCSVRow helpers are implemented inline in tCSV.h, below the
// "Implementation only below this line." marker.
//
// Copyright (c) 2026 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tFundamentals.h>
#include <System/tCSV.h>
#include <System/tFile.h>
namespace tSystem
{


bool tCSV::LoadFile(const tString& file)
{
	// Only load files that are actually CSV (i.e. whose extension maps to the CSV filetype). If the file is not a
	// CSV file, discard any previous content and leave the document empty.
	if (tGetFileType(file) != tFileType::CSV)
	{
		Rows.Empty();
		return false;
	}

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

	// Drop a UTF-8 byte order marker if the text starts with one. Some producers (notably Windows Excel) write a BOM
	// (U+FEFF, the three bytes EF BB BF) before the first character. RFC 4180 says nothing about it, so it must not
	// leak into the first cell. Only a leading BOM is removed; U+FEFF elsewhere is left untouched.
	tString content(text);
	if
	(
		(content.Length() >= 3) &&
		(uint8(content[0]) == 0xEF) &&
		(uint8(content[1]) == 0xBB) &&
		(uint8(content[2]) == 0xBF)
	)
	{
		content.ExtractLeft(3);
	}

	// Normalise line endings so that only '\n' separates rows. Handles Windows (CRLF), old-Mac (CR) and Unix (LF).
	tString normalised(content);
	normalised.Replace("\r\n", "\n");
	normalised.Replace("\r", "\n");

	// First pass: split the normalised text into the raw row strings, skipping blank rows.
	tList<tStringItem> rowStrings;
	tStd::tExplode(rowStrings, normalised, '\n');

	// Second pass: split each non-blank row into quote-aware items and store the resulting row. The items are
	// created and appended into the (owning) row's item list, so they are freed along with the row.
	for (tStringItem* rowString = rowStrings.First(); rowString; rowString = rowString->Next())
	{
		if (rowString->IsEmpty())
			continue;

		tCSVRow* row = new tCSVRow();
		tStd::tExplode(row->Items, *rowString, ',', '"');
		Rows.Append(row);
	}
}


bool tCSV::Set(const tString& value, int row, int col)
{
	if ((row < 0) || (col < 0))
		return false;

	// If the row does not exist yet, append empty rows until it does. New rows take the current width of the
	// document (or col+1 if the document is narrower than that) so the cell being created exists.
	int width = GetNumColumns();
	if (width < col + 1)
		width = col + 1;

	while (GetNumRows() < row + 1)
	{
		tCSVRow* newRow = new tCSVRow();
		for (int i = 0; i < width; ++i)
			newRow->Items.Append(new tStringItem());
		Rows.Append(newRow);
	}

	// If the column does not exist in every row yet, pad every row with empty items until it does.
	for (tCSVRow* r = Rows.First(); r; r = r->Next())
		while (r->ItemCount() < col + 1)
			r->Items.Append(new tStringItem());

	// Replace the cell.
	tStringItem* cell = Row(row)->Item(col);
	if (!cell)
		return false;

	cell->Set(value);
	return true;
}


bool tCSV::SetRow(const tList<tStringItem>& items, int row)
{
	if (row < 0)
		return false;

	// If the row does not exist yet, append empty rows until it does (they are brought up to the final width by
	// the padding pass below).
	while (GetNumRows() < row + 1)
		Rows.Append(new tCSVRow());

	// Delete the row's existing items and replace them with copies of the supplied items (copies, so the supplied
	// list and its ownership are left untouched).
	tCSVRow* target = Row(row);
	if (!target)
		return false;

	target->Items.Empty();
	for (tStringItem* item = items.First(); item; item = item->Next())
		target->Items.Append(new tStringItem(*item));

	// Keep the document consistent: the widest row wins and every shorter row is padded with empty items.
	int width = 0;
	for (tCSVRow* r = Rows.First(); r; r = r->Next())
		if (r->ItemCount() > width)
			width = r->ItemCount();

	for (tCSVRow* r = Rows.First(); r; r = r->Next())
		while (r->ItemCount() < width)
			r->Items.Append(new tStringItem());

	return true;
}


bool tCSV::SetColumn(const tList<tStringItem>& items, int col)
{
	if (col < 0)
		return false;

	// If the document does not have a row for every supplied item, append empty rows so every value has a home
	// (new rows are brought up to the final width by the padding pass below).
	int neededRows = GetNumRows();
	if (neededRows < items.GetNumItems())
		neededRows = items.GetNumItems();

	while (GetNumRows() < neededRows)
		Rows.Append(new tCSVRow());

	// Make sure every row has a cell in the target column.
	for (tCSVRow* r = Rows.First(); r; r = r->Next())
		while (r->ItemCount() < col + 1)
			r->Items.Append(new tStringItem());

	// Replace the column: cells take the supplied values in order, and any cells left over are emptied.
	tStringItem* value = items.First();
	for (tCSVRow* r = Rows.First(); r; r = r->Next())
	{
		tStringItem* cell = r->Item(col);
		if (value)
		{
			cell->Set(*value);
			value = value->Next();
		}
		else
			cell->Set(tString());
	}

	return true;
}


void tCSV::AppendCSVField(tString& dst, const tString& field)
{
	for (int i = 0; i < field.Length(); ++i)
	{
		char c = field[i];
		if ((c == ',') || (c == '"') || (c == '\r') || (c == '\n'))
		{
			tString quoted(field);
			quoted.Replace("\"", "\"\"");
			dst.Append('"');
			dst.Append(quoted);
			dst.Append('"');
			return;
		}
	}

	dst.Append(field);
}


bool tCSV::SaveFile(const tString& file) const
{
	// Only save to files that are actually CSV (i.e. whose extension maps to the CSV filetype).
	if (tGetFileType(file) != tFileType::CSV)
		return false;

	// RFC 4180: comma-separated fields, double-quote quoting (embedded quotes doubled), records separated by CRLF,
	// and every record having the same number of fields. The standard does not mandate a character encoding or a
	// BOM, so the file is written as plain UTF-8 with no BOM.

	// Every record must have the same number of fields, so find the widest row first.
	int width = 0;
	for (tCSVRow* r = Rows.First(); r; r = r->Next())
		if (r->ItemCount() > width)
			width = r->ItemCount();

	tString text;
	for (tCSVRow* r = Rows.First(); r; r = r->Next())
	{
		for (int i = 0; i < width; ++i)
		{
			if (i)
				text.Append(',');
			tStringItem* cell = r->Item(i);
			AppendCSVField(text, cell ? tString(*cell) : tString());
		}
		text.Append("\r\n");
	}

	return tCreateFile(file, (char8_t*)text.Chr(), text.Length(), false);
}


}
