#pragma once
#define project(projStr) namespace ProjectTacentVersion { extern int Major, Minor, Revision; struct Parser { Parser(const char*);  }; static Parser parser(#projStr); }
project("Tacent" "VERSION" "0.8.0" "LANGUAGES" "C" "CXX")
