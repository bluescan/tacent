#pragma once
#define set(verStr) namespace tVersion { extern int Major, Minor, Revision; struct Parser { Parser(const char*);  }; static Parser parser(#verStr); }

set("TACENT_VERSION" "0.8.12")

#undef set
