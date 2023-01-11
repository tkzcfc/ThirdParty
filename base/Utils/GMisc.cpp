#include "GMisc.h"
#include "../Algorithm/GRandom.h"
#include <time.h>
#include <assert.h>
#include "../Core/GLog.h"

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
			LogError() << "range error min:" << min << " max:" << max;
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

	void fatal(const std::string& reason, const char* filename, int line)
	{
		LogFatal() << "fatal error:" << reason << " , file:" << filename << " : " << line;
		::exit(-1);
	}
}
