#include "GMisc.h"
#include "../Algorithm/GRandom.h"
#include <time.h>
#include <assert.h>
#include "easylogging++.h"

namespace GMisc
{
	uint32_t uniqueUint32_t()
	{
		static uint32_t uniqueUint32Seed = 0;
		uniqueUint32Seed++;
		return uniqueUint32Seed;
	}


	uint32_t randomUint32_t(uint32_t min, uint32_t max)
	{
		auto now = (unsigned int)::time(NULL);
		static GRandom random(now, now + 1);
	
		if (min > max)
		{
			assert(0);
			LOG(ERROR) << "range error min:" << min << " max:" << max;
		}

		if (min == max)
		{
			return min;
		}
		
		return random.random(min, max);
	}

	int64_t timestamp()
	{
		auto now = ::time(NULL);
		return (int64_t)now;
	}

	std::string logTag(const char* name)
	{
		std::string tag = name;
		tag.append("    ");
		return tag;
	}
}
