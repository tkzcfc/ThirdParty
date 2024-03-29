﻿#pragma once

// https://github.com/preshing/RandomSequence

#include <stdint.h>

class GRandom
{
public:
	GRandom(unsigned int seedBase, unsigned int seedOffset);

	// @return [0, 1.0]
	double random0_1();

	// @return [min, max]
	uint32_t random(uint32_t min, uint32_t max);

private:

	unsigned int next();

	unsigned int next(unsigned int max);

	// @return [min,max)
	unsigned int range(unsigned int min, unsigned int max);

private:
	unsigned int m_index;
	unsigned int m_intermediateOffset;
};