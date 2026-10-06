// tCSV.h
//
// A CSV (comma-separated values) document container. It parses CSV using a comma (',') as the field divider and the
// double-quote ('"') as the field quote character, and stores the result as an ordered list of rows, where each row
// is an ordered list of items (cells), and each item is a tStringItem. The container owns all of its rows (and their
// items) and frees them when it is destroyed.
//
// Data can be loaded from a file or a string, read with Get (a single cell) or GetRow / GetColumn (filling a
// caller-supplied list), modified with Set (a single cell), SetRow (an entire row), or SetColumn (an entire column),
// and written back out with SaveFile. After every Set / SetRow / SetColumn call the document is consistent: every
// row has the same number of items. New rows are appended with empty items when needed, short rows are padded with
// empty items, and SetRow / SetColumn replace (delete) the existing data of the row or column they target.
//
// GetNumRows / GetNumColumns report the size, and out-of-range reads are safe: Get returns an empty tString, and
// GetRow / GetColumn report failure leaving the supplied list unchanged.
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

#pragma once
#include <Foundation/tFundamentals.h>
#include <Foundation/tString.h>
#include <Foundation/tList.h>
namespace tSystem
{


// tCSVRow holds the parsed columns of a single CSV row. It is a plain tList of tStringItems (the quote-aware field
// components) so that it participates in normal list ownership and teardown.
struct tCSVRow : public tLink<tCSVRow>
{
	// Items are created in the exact order they appear in the source row.
	tList<tStringItem> Items;

	int ItemCount() const																								{ return Items.GetNumItems(); }
	tStringItem* Item(int item) const;
};


// tCSV parses a CSV document (comma divider, double-quote quoting) and exposes its rows and columns. Use GetNumRows /
// GetNumColumns for size, Get for a single cell, and GetRow / GetColumn to fill a caller-supplied list. Documents can
// be mutated with Set / SetRow / SetColumn and written back out with SaveFile.
class tCSV
{
public:
	tCSV()																												{ }
	tCSV(const tString& file)																							{ LoadFile(file); }

	// LoadFile reads the file as UTF-8 and parses it. LoadString parses the supplied text directly. Line endings are
	// normalised (CRLF, LF and bare CR all separate records), blank lines are skipped, and a leading UTF-8 byte order
	// marker (EF BB BF), if present, is dropped rather than treated as part of the first cell.
	bool LoadFile(const tString& file);
	void LoadString(const tString& text);

	// Size accessors. GetNumColumns returns the largest number of items in any row.
	int GetNumRows() const																								{ return Rows.GetNumItems(); }
	int GetNumColumns() const;

	// Data access. Out-of-range reads are safe: Get returns an empty tString, and GetRow / GetColumn return false and
	// leave the supplied list unchanged.
	tString Get(int row, int col) const;
	bool GetRow(tList<tStringItem>& columns, int row) const;
	bool GetColumn(tList<tStringItem>& cells, int col) const;

	// Data mutation. The row and column parameters are zero-based. Every call leaves the document in a consistent
	// state where every row has the same number of items: new rows are appended with empty items when needed, and
	// short rows are padded with empty items. SetRow deletes the target row's existing items and replaces them with
	// copies of the supplied items. SetColumn empties the target column in every row, fills it with the supplied
	// values in order (appending empty rows if there are more items than rows), and leaves any cells without a
	// supplied value empty. Negative indices are rejected and false is returned.
	bool Set(const tString& value, int row, int col);
	bool SetRow(const tList<tStringItem>& items, int row);
	bool SetColumn(const tList<tStringItem>& items, int col);

	// Saves the document to file in RFC 4180 format: comma-separated fields, fields that contain a comma, a
	// double-quote, or a line break are enclosed in double-quotes with embedded double-quotes doubled, and records are
	// separated by CRLF. RFC 4180 does not mandate a character encoding or a BOM, so the file is written as plain UTF-8
	// with no BOM. Every record is written with the same number of fields (the widest row wins; shorter rows are padded
	// with empty fields). Returns true on success.
	bool SaveFile(const tString& file) const;

private:
	// Rows are stored in the order they appear in the document, one per non-empty source line.
	tList<tCSVRow> Rows;

	// Returns the row at index i, or nullptr if out of range.
	tCSVRow* Row(int i) const;

	// Appends a single RFC 4180 field to dst. A field that contains a comma, a double-quote, or a line break is
	// enclosed in double-quotes, with any embedded double-quotes doubled. All other fields are written as-is.
	static void AppendCSVField(tString& dst, const tString& field);
};


}


// Implementation only below this line.


inline tStringItem* tSystem::tCSVRow::Item(int item) const
{
	if ((item < 0) || (item >= ItemCount()))
		return nullptr;

	int index = item;
	for (tStringItem* cell = Items.First(); cell; cell = cell->Next())
		if (index-- == 0)
			return cell;

	return nullptr;
}


inline int tSystem::tCSV::GetNumColumns() const
{
	int maxColumns = 0;
	for (tCSVRow* row = Rows.First(); row; row = row->Next())
	{
		int count = row->ItemCount();
		if (count > maxColumns)
			maxColumns = count;
	}

	return maxColumns;
}


inline tString tSystem::tCSV::Get(int row, int col) const
{
	tCSVRow* source = Row(row);
	if (!source)
		return tString();

	tStringItem* cell = source->Item(col);
	if (!cell)
		return tString();

	return tString(*cell);
}


inline bool tSystem::tCSV::GetRow(tList<tStringItem>& columns, int row) const
{
	tCSVRow* source = Row(row);
	if (!source)
		return false;

	columns.Empty();
	for (int i = 0; i < source->ItemCount(); ++i)
	{
		tStringItem* cell = source->Item(i);
		columns.Append(new tStringItem(*cell));
	}

	return true;
}


inline bool tSystem::tCSV::GetColumn(tList<tStringItem>& cells, int col) const
{
	// A column is in range only if at least one row has a cell in that position.
	if ((col < 0) || (col >= GetNumColumns()))
		return false;

	cells.Empty();
	for (tCSVRow* row = Rows.First(); row; row = row->Next())
	{
		tStringItem* cell = row->Item(col);
		if (cell)
			cells.Append(new tStringItem(*cell));
	}

	return true;
}


inline tSystem::tCSVRow* tSystem::tCSV::Row(int i) const
{
	if ((i < 0) || (i >= GetNumRows()))
		return nullptr;

	int index = i;
	for (tCSVRow* row = Rows.First(); row; row = row->Next())
		if (index-- == 0)
			return row;

	return nullptr;
}
