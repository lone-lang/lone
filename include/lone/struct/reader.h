#ifndef LONE_STRUCT_READER_HEADER
#define LONE_STRUCT_READER_HEADER

#include <lone/struct/bytes.h>

struct lone_reader {
	int file_descriptor;
	struct {
		struct lone_bytes bytes;
		struct {
			size_t read;
			size_t write;
		} position;
	} buffer;
	int error;
};

#endif /* LONE_STRUCT_READER_HEADER */
