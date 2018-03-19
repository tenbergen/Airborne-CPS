#pragma once

#include <string.h>

// @author nstemmle
namespace strutil {
	// Creates the file path that is relative to the plugin's running directory for the specified relative file name
	// with the result being stored in the supplied in_buffer
	// Should handle cross-platform differences but it's not tested
	void buildFilePath(char * const inBuffer, char const * const fileName, char const * const pluginPath);
}