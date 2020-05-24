# install_headers('Inc/System/tChunk.h')
foundation_sources =												\
[
	'Src/tAssert.cpp',
	'Src/tBitArray.cpp',
	'Src/tMemory.cpp',
	'Src/tPlatform.cpp',
	'Src/tPool.cpp',
	'Src/tStandard.cpp',
	'Src/tString.cpp',
	'Src/tUnits.cpp',
]
tacentfoundationlib = static_library								\
(
	'foundation', foundation_sources,
	include_directories : 'Inc',
	install : true
)
