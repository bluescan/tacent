// tScript.cpp
//
// This module contains script file readers and writers. Tacent supports two text script formats. The main one is in
// the spirit of Church's lambda calculus and used 'symbolic expressions'. ex. [a b c]. See tExpression. I personally
// find this notation much more elegant than stuff like the lame XML notation. This site agrees:
// http://c2.com/cgi/wiki?XmlIsaPoorCopyOfEssExpressions
// The tExpression reader class in this file parses 'in-place'. That is, the entire file is just read into memory once
// and accessed as const data. This reduces memory fragmentation but may have made implementation more complex. 
//
// The second format is a functional format. ex. a(b,c) See tFunExtression.
//
// Copyright (c) 2006, 2017, 2019, 2020, 2022-2024 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tString.h>
#include "System/tFile.h"
#include "System/tScript.h"
using namespace tMath;


namespace tScript
{
	// Block comment begin and end characters. Putting them here in case we need to change them (again).
	static char BCB = '{';
	static char BCE = '}';
};
using namespace tScript;


tExpression tExpression::Car() const
{
	tAssert( IsValid() );

	const char8_t* c = ExprData + 1;

	if (*c == '\0')
		return tExpression();

	int lineCount;
	c = EatWhiteAndComments(c, lineCount);

	// Look for empty list.
	if (*c == ']')
		return tExpression();

	return tExpression(c, LineNumber+lineCount);
}


tExpression tExpression::CarCdrN(int n) const
{
	tExpression e = Car();

	for (int i = 0; i < n; i++)
	{
		if (!e.Valid())
			return e;

		e = e.Next();
	}

 	return e;
}


tExpression tExpression::Next() const
{
	tAssert( IsValid() );

	const char8_t* c = ExprData;
	int count = 0;
	int lineNum = LineNumber;

	if (!IsAtom())
	{
		// Skip the expression by counting square brackets till we are at zero again. Opening bracket adds 1, closing subtracts 1. Simple.
		// @todo Slight bug here. If there is a string inside the expression with []s, they must match.
		do
		{
			if (*c == '[')
				count++;

			if (*c == ']')
				count--;

			if (*c == '\0')
				return tExpression();

			if (*c == '\r')
				lineNum++;

			c++;
		}
		while (count);
	}
	else
	{
		// It's an atom. If it begins with a quote we must find where it ends.
		if (*c == '"')
		{
			c++;

			// Keep going until next quote.
			c = tStd::tStrchr(c, '"');

			if (!c)
			{
				if (lineNum != -1)
					throw tScriptError("Begin quote found but no end quote on line %d.", lineNum);
				else
					throw tScriptError("Begin quote found but no end quote.");
			}
			c++;
		}

		// If it begins with an open bracket (not a square bracket, a British bracket... a parenthesis in "American") we must find the closing bracket.
		else if (*c == '(')
		{
			c++;

			// Keep going until closing bracket.
			c = tStd::tStrchr(c, ')');

			if (!c)
			{
				if (lineNum != -1)
					throw tScriptError("Opening bracket found but no closing bracket on line %d.", lineNum);
				else
					throw tScriptError("Opening bracket found but no closing bracket.");
			}
			c++;
		}
		else
		{
			// The ';' and BCB should also be terminators for the current argument so that EatWhiteAndComments will get everything.
			char c1 = *c;
			while ((c1 != ' ') && (c1 != '\t') && (c1 != '[') && (c1 != ']') && (c1 != '\0') && (c1 != ';') && (c1 != BCB) && (c1 != '"'))
			{
				c++;

				// Newline is also considered a terminator.
				if (c1 == '\n')
				{
					lineNum++;
					break;
				}
				c1 = *c;
			}
		}
	}

	int lineCount;
	c = EatWhiteAndComments(c, lineCount);
	lineNum += lineCount;

	if ((*c == ']') || (*c == '\0'))
		// We're at the end of the argument list.
		return tExpression();

	return tExpression(c, lineNum);
}


bool tExpression::IsAtom() const
{
	if (ExprData && (*ExprData != '['))
		return true;
	else
		return false;
}


tString tExpression::GetExpressionString() const
{
	tAssert( IsValid() );
	if (!ExprData)
		return tString();

	if (IsAtom())
		return GetAtomString();

	tAssert(*ExprData == '[');
	const char8_t* start = ExprData;
	const char8_t* end = start;
	int bracketCount = 1;
	while (*++end)
	{
		if (*end == '[')
			bracketCount++;
		else if (*end == ']' && !--bracketCount)
			break;
	}

	if (*end)
		end++;
	else
		throw tScriptError(LineNumber, "Begin bracket found but no end bracket.");

	// Creates a tString full of '\0's. Note that 'end' is one passed the ending ']' so numChars is correct.
	int numChars = int(end - start);
	tString estr(numChars);
	tStd::tMemcpy(estr.Text(), start, numChars);
	return estr;
}


tString tExpression::GetAtomString() const
{
	if (!IsAtom())
		throw tScriptError(LineNumber, "Atom expected near: %s", GetContext().Pod());

	const char8_t* start;
	const char8_t* end;
	if (*ExprData == '"')
	{
		start = ExprData + 1;
		end = tStd::tStrchr(start, '"');

		// Keep going until next quote.
		if (!end)
			throw tScriptError(LineNumber, "Begin quote found but no end quote.");
	}

	// It may be a tuple atom in form (a, b, c)
	else if (*ExprData == '(')
	{
		start = ExprData;
		end = tStd::tStrchr(start, ')');

		// Keep going until next paren.
		if (end)
			end++;
		else
			throw tScriptError(LineNumber, "Begin paren found but no end paren.");
	}
	else
	{
		start = ExprData;
		end = ExprData;
		while
		(
			(*end != ' ') && (*end != '\t') && (*end != '[') && (*end != ']') && (*end != '\0') &&
			(*end != '\r') && (*end != '\n') && (*end != ';') && (*end != BCB) && (*end != '"')
		) end++;
	}

	// Creates a tString full of '\0's. End is one bigger so we get the right numChars.
	int numChars = int(end - start);
	tString atom(numChars);
	tStd::tStrncpy(atom.Text(), start, numChars);

	return atom;
}


tString tExpression::GetAtomTupleString() const
{
	if (!IsAtom())
		throw tScriptError(LineNumber, "Atom expected near: %s", GetContext().Pod());

	if (*ExprData != '(')
		throw tScriptError(LineNumber, "Tuple atom expected near: %s", GetContext().Pod());

	const char8_t* start = ExprData + 1;
	const char8_t* end = tStd::tStrchr(start, ')');

	// If no end paren was found we're in trouble.
	if (!end)
		throw tScriptError(LineNumber, "Opening paren but no corresponding closing paren.");

	// Creates a tString full of '\0's.
	int numChars = int(end - start);
	tString tuple(numChars);
	tStd::tStrncpy(tuple.Text(), start, numChars);

	// Now remove the '(', the ')', and any spaces.
	tuple.RemoveAny("() ");
	return tuple;
}


tVector2 tExpression::GetAtomVector2() const
{
	// Vectors should be of the form (x, y). Spaces or not after and before the comma.
	tString str = GetAtomTupleString();
	tVector2 v;

	tString component = str.ExtractLeft(',');
	v.x = component.GetAsFloat();

	v.y = str.GetAsFloat();
	return v;
}


tVector3 tExpression::GetAtomVector3() const
{
	// Vectors should be of the form (x, y, z). Spaces or not after and before the commas.
	tString str = GetAtomTupleString();
	tVector3 v;

	tString component = str.ExtractLeft(',');
	v.x = component.GetAsFloat();

	component = str.ExtractLeft(',');
	v.y = component.GetAsFloat();

	v.z = str.GetAsFloat();
	return v;
}


tVector4 tExpression::GetAtomVector4() const
{
	// Vectors should be of the form (x, y, z, w). Spaces or not after and before the commas.
	tString str = GetAtomTupleString();
	tVector4 v;

	tString component = str.ExtractLeft(',');
	v.x = component.GetAsFloat();

	component = str.ExtractLeft(',');
	v.y = component.GetAsFloat();

	component = str.ExtractLeft(',');
	v.z = component.GetAsFloat();

	v.w = str.GetAsFloat();
	return v;
}


tQuaternion tExpression::GetAtomQuaternion() const
{
	// Quaternions should be of the form (w, x, y, z). Spaces or not after and before the commas.
	tString str = GetAtomTupleString();
	tQuaternion q;

	tString component = str.ExtractLeft(',');
	q.x = component.GetAsFloat();

	component = str.ExtractLeft(',');
	q.y = component.GetAsFloat();

	component = str.ExtractLeft(',');
	q.z = component.GetAsFloat();

	q.w = str.GetAsFloat();
	return q;
}


tMatrix2 tExpression::GetAtomMatrix2() const
{
	// Matrices should be of the form (a11, a21, a12, a22). Spaces or not after and before the commas.
	tString str = GetAtomTupleString();
	tMatrix2 m;

	tString component = str.ExtractLeft(',');
	m.C1.x = component.GetAsFloat();

	component = str.ExtractLeft(',');
	m.C1.y = component.GetAsFloat();

	component = str.ExtractLeft(',');
	m.C2.x = component.GetAsFloat();

	m.C2.y = str.GetAsFloat();
	return m;
}


tMatrix4 tExpression::GetAtomMatrix4() const
{
	// Matrices should be of the form (a11, a21, a31, a41, a12, a22, ..., a44).
	// Spaces or not after and before the commas.
	tString str = GetAtomTupleString();
	tMatrix4 m;

	tString component = str.ExtractLeft(',');
	m.E[0] = component.GetAsFloat();

	for (int e = 1; e < 15; e++)
	{
		component = str.ExtractLeft(',');
		m.E[e] = component.GetAsFloat();
	}

	m.E[15] = str.GetAsFloat();
	return m;
}


tColour4b tExpression::GetAtomColour() const
{
	// Colours should be of the form (r, g, b, a). Spaces or not after and before the commas.
	tString str = GetAtomTupleString();
	tColour4b c;

	tString component = str.ExtractLeft(',');
	c.R = component.GetAsInt();

	component = str.ExtractLeft(',');
	c.G = component.GetAsInt();

	component = str.ExtractLeft(',');
	c.B = component.GetAsInt();

	c.A = str.GetAsInt();
	return c;
}


const char8_t* tExpression::EatWhiteAndComments(const char8_t* c, int& lineCount)
{
	// There are two types of comment. Single-line comments using a semi-colon go to the end of the current line.
	// Block (multi-line) comments are delimited with { and }. Note that { } are still allowed inside a string
	// such as "this { string" without being considered as begin or end comment markers.
	bool inSingleLineComment = false;
	int inMultiLineComment = 0;
	bool inString = false;

	lineCount = 0;

	while
	(
		(*c == ' ') || (*c == '\t') || (*c == '\n') || (*c == '\r') || (*c == 9) ||
		(*c == ';') || (*c == BCB) || (*c == BCE) || inSingleLineComment || inMultiLineComment
	)
	{
		if ((*c == BCB) && !inSingleLineComment && !inString)
			inMultiLineComment++;

		else if ((*c == BCE) && !inSingleLineComment && inMultiLineComment && !inString)
			inMultiLineComment--;

		else if ((*c == ';') && !inMultiLineComment)
			inSingleLineComment = true;

		else if (inSingleLineComment && ((*c == '\r') || (*c == '\n') || (*c == '\0')))
			inSingleLineComment = false;

		else if (inMultiLineComment && (*c == '"'))
			inString = !inString;

		if (*c == '\n')
			lineCount++;

		c++;
	}

	return c;
}


tString tExpression::GetContext() const
{
	tString context(ContextSize);

	// We're allowed to do this because as soon as strncpy sees a 0 in the src string it stops reading from it.
	tStd::tStrncpy(context.Text(), ExprData, ContextSize);
	context.SetLength( tStd::tStrlen(context.Text()) );

	int newLine = context.FindChar('\r');

	// Only go to the end of the current line. The set-length will end up removing the '\r' cuz it's one less.
	if (newLine != -1)
		context.SetLength(newLine);

	return context;
}


tExprReader::tExprReader(int argc, char** argv) :
	tExpression(),
	ExprBuffer(nullptr)
{
	// Here we're just concatenating all the argv strings into one.
	tString scriptString = "[";

	for (int i = 0; i < argc; i++)
	{
		tString itemToAdd(argv[i]);
		if (itemToAdd.IsEmpty())
			continue;

		// OK, here we need to interpret the item as a string atom if there is a space. If there is a space, we may
		// need to add quotes if they aren't already there. Normally there won't be any spaces because spaces are
		// precisely what separates the argv arguments from each other. However, sometimes argv[0] can be a single
		// string something like "C:\Program File\SomeDir\SomeFile.txt" and it needs to be treated as a single item even
		// though it has a space.
		//
		// The reason we check for the double quotes is because it is perfectly ok for the command line to contain escaped
		// quotes, in which case the work of inserting them is already done.
		//
		// There is one last caveat: the command line: a.exe [b "c d"] will come in as: 'a.exe', '[b', 'c d]' because there
		// was no space after the last double quote. It turns out it is safe to ignore this if the atom does not have a
		// space, and if it does, we can simply replace the FIRST occurence of a ']' with a '"]' We don't need to handle
		// multiple close brackets and subsequent open brackets are also ok because the tScript syntax demands a space
		// between all arguments. Come to think of it we also need to deal with ["a b" c] going to '[a b' and 'c]' but
		// whatever, this is getting too complicated, I'm just going to require escaped quotes in the command line.
		if ((itemToAdd.FindChar(' ') != -1) && (itemToAdd[0] != '"') && (itemToAdd[itemToAdd.Length()-1] != '"'))
		{
			// Add a beginning quote.
			itemToAdd = "\"" + itemToAdd + "\"";
		}

		scriptString += itemToAdd;

		// Add a space if it's not the last argv string.
		if (i < argc-1)
			scriptString += " ";
	}

	scriptString += "]";

	ExprBuffer = new char8_t[scriptString.Length() + 1];
	tStd::tStrcpy(ExprBuffer, scriptString);

	ExprData = ExprBuffer;
	LineNumber = 1;
}


void tExprReader::Load(const tString& name, bool isFile)
{
	Clear();
	if (name.IsEmpty())
		return;

	if (isFile)
	{
		tFileHandle file = tSystem::tOpenFile(name, "rb");

		// @todo Consider just becoming an invalid expression.
		tAssert(file);

		int fileSize = tSystem::tGetFileSize(file);

		// Create a buffer big enough for the file, the uber []'s, two line-endings (one for each square bracket), and a terminating 0.
		int bufferSize = fileSize + 7;

		ExprBuffer = new char8_t[bufferSize];
		ExprBuffer[0] = '[';
		ExprBuffer[1] = '\r';
		ExprBuffer[2] = '\n';

		// Load the entire thing into memory.
		int numRead = tSystem::tReadFile(file, (uint8*)(ExprBuffer+3), fileSize);
		if (numRead != fileSize)
			throw tScriptError("Cannot read file [%s].", name.Pod());
		tSystem::tCloseFile(file);

		ExprBuffer[bufferSize-4] = '\r';
		ExprBuffer[bufferSize-3] = '\n';
		ExprBuffer[bufferSize-2] = ']';
		ExprBuffer[bufferSize-1] = '\0';
	}
	else
	{
		int stringSize = name.Length();
		int bufferSize = stringSize + 7;
		ExprBuffer = new char8_t[bufferSize];

		ExprBuffer[0] = '[';
		ExprBuffer[1] = '\r';
		ExprBuffer[2] = '\n';
		tStd::tStrcpy(ExprBuffer+3, name);
		ExprBuffer[bufferSize-4] = '\r';
		ExprBuffer[bufferSize-3] = '\n';
		ExprBuffer[bufferSize-2] = ']';
		ExprBuffer[bufferSize-1] = '\0';
	}

	LineNumber = 1;
	ExprData = EatWhiteAndComments(ExprBuffer, LineNumber);
}


tExprWriter::tExprWriter(const tString& filename) :
	CurrIndent(0),
	TabWidth(0)
{
	ExprFile = tSystem::tOpenFile(filename, "wt");

	if (!ExprFile)
		throw tScriptError("Cannot open file [%s].", tPod(filename));
}


void tExprWriter::BeginExpression()
{
	char beg[] = "[ ";
	int numWritten = tSystem::tWriteFile(ExprFile, beg, 2);
	if (numWritten != 2)
		throw tScriptError("Cannot write to script file.");
}


void tExprWriter::EndExpression()
{
	char end[] = "] ";
	int numWritten = tSystem::tWriteFile(ExprFile, end, 2);
	if (numWritten != 2)
		throw tScriptError("Cannot write to script file.");
}


void tExprWriter::WriteAtom(const tString& atom)
{
	char qu = '"';

	bool hasSpace = true;
	if (atom.FindChar(' ') == -1)
		hasSpace = false;

	// Here we determine whether to use quotes. If the atom has a space, we need them.
	bool useQuotes = hasSpace || atom.IsEmpty();
	int numWritten = 0;
	if (useQuotes)
		numWritten += tSystem::tWriteFile(ExprFile, &qu, 1);

	int atomLen = atom.Length();
	numWritten += tSystem::tWriteFile(ExprFile, atom, atomLen);
	if (useQuotes)
		numWritten += tSystem::tWriteFile(ExprFile, &qu, 1);

	char sp = ' ';
	numWritten += tSystem::tWriteFile(ExprFile, &sp, 1);

	if (numWritten != (1 + (useQuotes ? 2 : 0) + atomLen))
		throw tScriptError("Cannot write atom [%s] to script file.", tPod(atom));
}


void tExprWriter::WriteAtom(const char* atom)
{
	char qu = '"';

	bool hasSpace = false;
	if (tStd::tStrchr(atom, ' '))
		hasSpace = true;

	// Here we determine whether to use quotes if necessary. If the atom is a tuple (a vector or matrix etc) then we do
	// not use quotes even if spaces are present.
	bool useQuotes = hasSpace;
	int numWritten = 0;
	if (useQuotes)
		numWritten += tSystem::tWriteFile(ExprFile, &qu, 1);

	int atomLen = tStd::tStrlen(atom);
	numWritten += tSystem::tWriteFile(ExprFile, atom, atomLen);
	if (useQuotes)
		numWritten += tSystem::tWriteFile(ExprFile, &qu, 1);

	char sp = ' ';
	numWritten += tSystem::tWriteFile(ExprFile, &sp, 1);

	if (numWritten != (1 + (useQuotes ? 2 : 0) + atomLen))
		throw tScriptError("Cannot write atom '%s' to script file.", atom);
}


void tExprWriter::WriteRaw(const tString& atom)
{
	int numWritten = 0;

	int atomLen = atom.Length();
	numWritten += tSystem::tWriteFile(ExprFile, atom, atomLen);

	char sp = ' ';
	numWritten += tSystem::tWriteFile(ExprFile, &sp, 1);

	if (numWritten != (1 + atomLen))
		throw tScriptError("Cannot write atom [%s] to script file.", tPod(atom));
}


void tExprWriter::WriteRaw(const char* atom)
{
	int numWritten = 0;

	int atomLen = tStd::tStrlen(atom);
	numWritten += tSystem::tWriteFile(ExprFile, atom, atomLen);

	char sp = ' ';
	numWritten += tSystem::tWriteFile(ExprFile, &sp, 1);

	if (numWritten != (1 + atomLen))
		throw tScriptError("Cannot write atom '%s' to script file.", atom);
}


void tExprWriter::WriteAtom(const bool atom)
{
	if (atom)
		WriteAtom("True");
	else
		WriteAtom("False");
}


void tExprWriter::WriteAtom(const uint32 atom)
{
	char val[36];
	tStd::tItoa(val, 36, atom, 10);
	WriteAtom(val);
}


void tExprWriter::WriteAtom(const uint64 atom)
{
	char val[48];
	tStd::tItoa(val, 48, atom, 10);
	WriteAtom(val);
}


void tExprWriter::WriteAtom(const int atom)
{
	char val[36];
	tStd::tItoa(val, 36, atom, 10);
	WriteAtom(val);
}


void tExprWriter::WriteAtom(const float atom, bool incBitRep)
{
	tString str;
	tSystem::tFtostr(str, atom, incBitRep);
	WriteAtom(str);
}


void tExprWriter::WriteAtom(const double atom, bool incBitRep)
{
	tString str;
	tSystem::tDtostr(str, atom, incBitRep);
	WriteAtom(str);
}


void tExprWriter::WriteAtom(const tVector2& v, bool incBitRep)
{
	tString str("(");
	for (int e = 0; e < 2; e++)
	{
		float f = v.E[e];
		if (tStd::tIsSpecial(f))
			f = 0.0f;

		char val[64];
		char* cval = val;
		cval += tsPrintf(cval, "%8.8f", f);

		// Add a trailing 0 because it looks better.
		if (*(cval-1) == '.')
		{
			*cval++ = '0';
			*cval = '\0';
		}
		if (incBitRep)
			cval += tsPrintf(cval, "#%08X", *((uint32*)&f));

		str += tString(val);
		if (e != 1)
			str += ", ";
	}
	str += ")";
	WriteRaw(str);
}


void tExprWriter::WriteAtom(const tVector3& v, bool incBitRep)
{
	tString str("(");
	for (int e = 0; e < 3; e++)
	{
		float f = v.E[e];
		if (tStd::tIsSpecial(f))
			f = 0.0f;

		char val[64];
		char* cval = val;
		cval += tsPrintf(cval, "%8.8f", f);

		// Add a trailing 0 because it looks better.
		if (*(cval-1) == '.')
		{
			*cval++ = '0';
			*cval = '\0';
		}
		if (incBitRep)
			cval += tsPrintf(cval, "#%08X", *((uint32*)&f));

		str += tString(val);
		if (e != 2)
			str += ", ";
	}
	str += ")";
	WriteRaw(str);
}


void tExprWriter::WriteAtom(const tVector4& v, bool incBitRep)
{
	tString str("(");
	for (int e = 0; e < 4; e++)
	{
		float f = v.E[e];
		if (tStd::tIsSpecial(f))
			f = 0.0f;

		char val[64];
		char* cval = val;
		cval += tsPrintf(cval, "%8.8f", f);

		// Add a trailing 0 because it looks better.
		if (*(cval-1) == '.')
		{
			*cval++ = '0';
			*cval = '\0';
		}
		if (incBitRep)
			cval += tsPrintf(cval, "#%08X", *((uint32*)&f));

		str += tString(val);
		if (e != 3)
			str += ", ";
	}
	str += ")";
	WriteRaw(str);
}


void tExprWriter::WriteAtom(const tQuaternion& q, bool incBitRep)
{
	tString str("(");
	for (int e = 0; e < 4; e++)
	{
		float f = q.E[e];
		if (tStd::tIsSpecial(f))
			f = 0.0f;

		char val[64];
		char* cval = val;
		cval += tsPrintf(cval, "%8.8f", f);

		// Add a trailing 0 because it looks better.
		if (*(cval-1) == '.')
		{
			*cval++ = '0';
			*cval = '\0';
		}
		if (incBitRep)
			cval += tsPrintf(cval, "#%08X", *((uint32*)&f));

		str += tString(val);
		if (e != 3)
			str += ", ";
	}
	str += ")";
	WriteRaw(str);
}


void tExprWriter::WriteAtom(const tMatrix2& m, bool incBitRep)
{
	tString str("(");
	for (int e = 0; e < 4; e++)
	{
		float f = m.E[e];
		if (tStd::tIsSpecial(f))
			f = 0.0f;

		char val[64];
		char* cval = val;
		cval += tsPrintf(cval, "%8.8f", f);

		// Add a trailing 0 because it looks better.
		if (*(cval-1) == '.')
		{
			*cval++ = '0';
			*cval = '\0';
		}
		if (incBitRep)
			cval += tsPrintf(cval, "#%08X", *((uint32*)&f));

		str += tString(val);
		if (e != 3)
			str += ", ";
	}
	str += ")";
	WriteRaw(str);
}


void tExprWriter::WriteAtom(const tMatrix4& m, bool incBitRep)
{
	tString str("(");
	for (int e = 0; e < 16; e++)
	{
		float f = m.E[e];
		if (tStd::tIsSpecial(f))
			f = 0.0f;

		char val[64];
		char* cval = val;
		cval += tsPrintf(cval, "%8.8f", f);

		// Add a trailing 0 because it looks better.
		if (*(cval-1) == '.')
		{
			*cval++ = '0';
			*cval = '\0';
		}
		if (incBitRep)
			cval += tsPrintf(cval, "#%08X", *((uint32*)&f));

		str += tString(val);
		if (e != 15)
			str += ", ";
	}
	str += ")";
	WriteRaw(str);
}


void tExprWriter::WriteAtom(const tColour4b& c)
{
	tString str("(");
	for (int e = 0; e < 4; e++)
	{
		char val[36];
		tStd::tItoa(val, 36, c.E[e], 10);

		str += val;
		if (e != 3)
			str += ", ";
	}
	str += ")";
	WriteRaw(str);
}


void tExprWriter::WriteComment(const char* comment)
{
	char sc[] = "; ";
	int numWritten = tSystem::tWriteFile(ExprFile, sc, 2);
	int commentLen = 0;
	if (comment)
	{
		commentLen = tStd::tStrlen(comment);
		numWritten += tSystem::tWriteFile(ExprFile, comment, commentLen);
	}

	if (numWritten != (2 + commentLen))
		throw tScriptError("Cannot write to script file.");

	NewLine();
}


void tExprWriter::WriteCommentLine(const char* comment)
{
	int numWritten = 0;
	int commentLen = 0;
	if (comment)
	{
		commentLen = tStd::tStrlen(comment);
		numWritten += tSystem::tWriteFile(ExprFile, comment, commentLen);
	}

	if (numWritten != commentLen)
		throw tScriptError("Cannot write to script file.");

	NewLine();
}


void tExprWriter::WriteCommentEnd()
{
	char sc[] = ">\n";
	sc[0] = BCE;
	int numWritten = tSystem::tWriteFile(ExprFile, sc, 2);
	if (numWritten != 2)
		throw tScriptError("Cannot write to script file.");
}


void tExprWriter::WriteCommentInlineBegin()
{
	char sc[] = "< ";
	sc[0] = BCB;
	int numWritten = tSystem::tWriteFile(ExprFile, sc, 2);
	if (numWritten != 2)
		throw tScriptError("Cannot write to script file.");
}


void tExprWriter::WriteCommentInline(const char* comment)
{
	int numWritten = 0;
	int commentLen = 0;
	if (comment)
	{
		commentLen = tStd::tStrlen(comment);
		numWritten += tSystem::tWriteFile(ExprFile, comment, commentLen);
	}

	if (numWritten != commentLen)
		throw tScriptError("Cannot write to script file.");
}


void tExprWriter::WriteCommentInlineEnd()
{
	char sc[] = " > ";
	sc[1] = BCE;
	int numWritten = tSystem::tWriteFile(ExprFile, sc, 3);
	if (numWritten != 3)
		throw tScriptError("Cannot write to script file.");
}


void tExprWriter::NewLine()
{
	char nl = '\n';
	char tb = '\t';

	int numWritten = tSystem::tWriteFile(ExprFile, &nl, 1);
	numWritten += WriteIndents();
	int expectedWritten = 1 + (TabWidth ? CurrIndent*TabWidth : CurrIndent);

	if (numWritten != expectedWritten)
		throw tScriptError("Cannot write to script file.");
}


// Next follow the types for the functional scripts of the form f(a, b).
tFunExpression::tFunExpression(const char8_t* function)
{
	// @todo There are better ways to do this.
	const int maxExpressionSize = 512;
	char buffer[maxExpressionSize];
	tStd::tStrncpy(buffer, (char*)function, maxExpressionSize);
	buffer[maxExpressionSize-1] = '\0';

	// We need to find the trailing ')' that is at the end of our expression. We do this by incrementing and decrementing
	// based on '(' or ')'
	char* beginningParen = (char*)tStd::tStrchr(buffer, '(');
	char* endParen = beginningParen;
	int openCount = 0;

	for (int c = 0; c < maxExpressionSize-1; c++)
	{
		if (*endParen == '(')
			openCount++;
		else if (*endParen == ')')
			openCount--;

		if (!openCount)
			break;

		endParen++;
	}

	if (openCount)
		throw tScriptError("Expression too long. Missing bracket? Max size is %d chars. Look for [%s]", maxExpressionSize, buffer);

	*endParen = '\0';
	int origLength = tStd::tStrlen(buffer);
	*beginningParen = '\0';

	// Now replace all occurences of space, tab, and comma with nulls. Double quote string arguments are handled correctly.
	bool inString = false;
	openCount = 0;
	for (int c = 0; c < origLength; c++)
	{
		int ch = buffer[c];
		if (ch == '"')
		{
			inString = !inString;
			buffer[c] = '\0';
		}

		if (ch == '(')
			openCount++;

		if (ch == ')')
			openCount--;

		if ((!inString) && (!openCount))
		{
			if ((ch == '\t') || (ch == ',') || (ch == '\n') || (ch == '\r') || (ch == 9) || (ch == ' '))
				buffer[c] = '\0';
		}
	}

	// We now have a string of the form: name00arga000argb0argc00. The function name is the first word.
	Function = buffer;
	int pos = Function.Length();

	// Now lets get arguments until there are no more.
	int endIdx = int(endParen-beginningParen);
	while (1)
	{
		while ((buffer[pos] == '\0') && (pos < endIdx))
			pos++;

		if (pos == endIdx)
			break;

		tStringItem* arg = new tStringItem( &buffer[pos] );
		Arguments.Append(arg);
		pos += arg->Length();
	}
}


void tFunScript::Load(const tString& fileName)
{
	Clear();
	tFileHandle file = tSystem::tOpenFile(fileName.Chars() , "rb");
	tAssert(file);

	// Create a buffer big enough for the file.
	int fileSize = tSystem::tGetFileSize(file);
	char8_t* buffer = new char8_t[fileSize + 1];

	// Load the entire thing into memory.
	int numRead = tSystem::tReadFile(file, (uint8*)buffer, fileSize);
	tAssert(numRead == fileSize);

	// This makes buffer a valid null terminated string.
	buffer[fileSize] = '\0';
	tSystem::tCloseFile(file);

	char8_t* currChar = EatWhiteAndComments(buffer);
	while (*currChar != '\0')
	{
		Expressions.Append(new tFunExpression(currChar));

		// Get to the next expression.
		currChar = tStd::tStrchr(currChar, '(');
		int openCount = 0;
		while (1)
		{
			if (*currChar == '(')
				openCount++;

			if (*currChar == ')')
				openCount--;

			currChar++;
			if (!openCount)
				break;
		}

		currChar = EatWhiteAndComments(currChar);
	}

	delete[] buffer;
}


void tFunScript::Save(const tString& fileName)
{
	tFileHandle file = tSystem::tOpenFile(fileName, "wt");

	if (!file)
		throw tScriptError("Cannot open file '%s'.", fileName.Chr());

	// All we need to do is traverse the expression list and write out each one.
	for (tFunExpression* exp = Expressions.First(); exp; exp = exp->Next())
	{
		tfPrintf(file, "%s(", exp->Function.Text());
		for (tStringItem* arg = exp->Arguments.First(); arg; arg = arg->Next())
		{
			tfPrintf(file, "%s", arg->Text());

			// If it's not the last argument we put a comma there.
			if (arg->Next())
				tfPrintf(file, ", ");
		}
		tfPrintf(file, ")\n");
	}

	tSystem::tCloseFile(file);
}


char8_t* tFunScript::EatWhiteAndComments(char8_t* c)
{
	bool inComment = false;
	while ((*c == ' ') || (*c == '\t') || (*c == '\n') || (*c == '\r') || (*c == 9) || (*c == '/') || inComment)
	{
		if (*c == '/')
			inComment = true;

		if ((*c == '\r') || (*c == '\n') || (*c == '\0'))
			inComment = false;

		c++;
	}

	return c;
}
