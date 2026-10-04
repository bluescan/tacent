// tCSV.h
//
// A passive CSV (comma-separated values) document container. It parses CSV using a comma (',') as the field divider
// and the double-quote ('"') as the field quote character, and stores the result as an ordered list of rows, where
// each row is an ordered list of columns (cells), and each cell is a tStringItem. The container owns all of its rows
// (and their columns) and frees them when it is destroyed.
//
// It is passive: it exposes a read-only view of the parsed data. There is no error state to check -- simply query the
// size with GetNumRows / GetNumColumns and read data with Get (a single cell) or GetRow / GetColumn (filling a
// caller-supplied list). An empty document has zero rows and zero columns, and out-of-range reads are safe: Get
// returns an empty tString, and GetRow / GetColumn report failure leaving the supplied list unchanged.
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
	// Columns are created in the exact order they appear in the source row.
	tList<tStringItem> Columns;

	int ColumnCount() const																								{ return Columns.GetNumItems(); }
	tStringItem* Column(int col) const;
};


// tCSV parses a CSV document (comma divider, double-quote quoting) and exposes a passive, read-only view of its rows
// and columns. Use GetNumRows / GetNumColumns for size, Get for a single cell, and GetRow / GetColumn to fill a
// caller-supplied list.
class tCSV
{
public:
	tCSV()																												{ }
	tCSV(const tString& file)																							{ LoadFile(file); }
	bool LoadFile(const tString& file);
	void LoadString(const tString& text);

	// Size accessors. GetNumColumns returns the largest number of columns in any row.
	int GetNumRows() const																								{ return Rows.GetNumItems(); }
	int GetNumColumns() const;

	// Data access. Out-of-range reads are safe: Get returns an empty tString, and GetRow / GetColumn return false and
	// leave the supplied list unchanged.
	tString Get(int row, int col) const;
	bool GetRow(tList<tStringItem>& columns, int row) const;
	bool GetColumn(tList<tStringItem>& cells, int col) const;

private:
	// Rows are stored in the order they appear in the document, one per non-empty source line.
	tList<tCSVRow> Rows;

	// Returns the row at index i, or nullptr if out of range.
	tCSVRow* Row(int i) const;
};


}


// Implementation only below this line.


inline tStringItem* tSystem::tCSVRow::Column(int col) const
{
	if ((col < 0) || (col >= ColumnCount()))
		return nullptr;

	int index = col;
	for (tStringItem* cell = Columns.First(); cell; cell = cell->Next())
		if (index-- == 0)
			return cell;

	return nullptr;
}


inline int tSystem::tCSV::GetNumColumns() const
{
	int maxColumns = 0;
	for (tCSVRow* row = Rows.First(); row; row = row->Next())
	{
		int count = row->ColumnCount();
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

	tStringItem* cell = source->Column(col);
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
	for (int i = 0; i < source->ColumnCount(); ++i)
	{
		tStringItem* cell = source->Column(i);
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
		tStringItem* cell = row->Column(col);
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
