#pragma once

#include <string.h>

// @author nstemmle
namespace str_util {
	// Creates the file path that is relative to the plugin's running directory for the specified relative file name
	// with the result being stored in the supplied in_buffer
	// Should handle cross-platform differences but it's not tested
	void BuildFilePath(char * const in_buffer, char const * const file_name, char const * const plugin_path);
}