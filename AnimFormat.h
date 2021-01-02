#pragma once

#include <stdio.h>

class AnimFormat
{
public:
	virtual void WriteData(FILE *dst_file) = 0;
};

