#pragma once

#include <ConcurrencySal.h>
#include <vector>

#include "Renderer.inc"

class DebugTextRenderer
{
public:
	DebugTextRenderer();
	~DebugTextRenderer();

	unsigned int AddDebugString(char* debug_string);
	void RemoveDebugString(unsigned int id);
	void Render();
	
private:
	bool render_ = true;
	std::vector<char*> debug_strings_;
};