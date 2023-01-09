
#include <stdint.h>
#include <string>

namespace GMisc
{
	uint32_t uniqueUint32_t();

	uint32_t randomUint32_t(uint32_t min, uint32_t max);

	int64_t timestamp();

	std::string logTag(const char* name);

	void fatal(const std::string& reason, const char* filename, int line);
}

#define G_FATAL(reason) GMisc::fatal(reason, __FILE__, __LINE__)

