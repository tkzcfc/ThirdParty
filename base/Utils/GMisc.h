
#include <stdint.h>
#include <string>

namespace GMisc
{
	uint32_t uniqueUint32_t();

	uint32_t randomUint32_t(uint32_t min, uint32_t max);

	int64_t timestamp();

	std::string logTag(const char* name);
}

