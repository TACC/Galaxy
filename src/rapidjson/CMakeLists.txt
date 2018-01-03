cmake_minimum_required (VERSION 2.8.11)
project (rapidjson)

install(FILES allocators.h document.h encodedstream.h encodings.h filereadstream.h
	filestream.h filewritestream.h memorybuffer.h memorystream.h prettywriter.h
	rapidjson.h reader.h stringbuffer.h writer.h DESTINATION include/rapidjson)

install(FILES error/en.h error/error.h DESTINATION include/rapidjson/error)

install(FILES internal/biginteger.h internal/diyfp.h internal/dtoa.h 
				internal/ieee754.h internal/itoa.h internal/meta.h
				internal/pow10.h internal/stack.h internal/strfunc.h 
				internal/strtod.h DESTINATION include/rapidjson/internal)
