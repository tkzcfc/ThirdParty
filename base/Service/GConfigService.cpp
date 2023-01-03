#include "GConfigService.h"
#include "Utils/GStringUtils.h"
#include "GApplication.h"
#include "GFileSystem.h"

uint32_t GConfigService::onInit()
{
	auto cfgFile = GFileSystem::getExeDirectory() + "cfg/config.ini";

	std::string cfgContent;
	if (!GFileSystem::readStringFromFile(cfgFile, cfgContent))
	{
		LOG(ERROR) << "Unable to find configuration file: " << cfgFile;
		return SCODE_START_FAIL_EXIT_APP;
	}

	std::vector<std::string> lines;
	StringUtils::split(cfgContent, "\n", lines);

	bool defineStart = false;
	std::vector<std::pair<std::string, std::string>> kvp;
	for (std::size_t i = 0; i < lines.size(); ++i)
	{
		auto line = StringUtils::trim(lines[i]);

		if (line.empty() || line[0] == ';' || line[0] == '#')
			continue;

		// 对以下格式去除尾部注释
		// xxx = abc ;注释
		// xxx = abc #注释
		auto pos = line.find_first_of(";#");
		if (pos != std::string::npos)
		{
			line = StringUtils::trim(line.substr(0, pos));
		}

		if (!defineStart)
		{
			auto valstr = line;
			std::transform(valstr.begin(), valstr.end(), valstr.begin(), ::tolower);
			if (valstr == "[define]")
			{
				defineStart = true;
				continue;
			}
		}
		else
		{
			if (line[0] == '[')
				break;

			std::pair<std::string, std::string> kv;
			pos = line.find("=");
			if (pos != line.npos)
			{
				if (pos > 0)
				{
					kv.first = StringUtils::trim(line.substr(0, pos));
				}
				kv.second = StringUtils::trim(line.substr(pos + 1));
			}

			if (kv.first.empty())
				continue;

			kvp.push_back(kv);
		}
	}

	for (auto& kv : kvp)
	{
		cfgContent = StringUtils::replaceString(cfgContent, "${" + kv.first + "}", kv.second);
	}

	auto newCfgFile = GFileSystem::getExeDirectory() + "tmp_cfg_" + GApplication::getInstance()->getAppName() + ".ini";

	if (!GFileSystem::writeTextFile(newCfgFile, cfgContent))
	{
		LOG(ERROR) << "Unable to generate temporary configuration file:: " << newCfgFile;
		return SCODE_START_FAIL_EXIT_APP;
	}

	m_reader = std::make_unique<GINIReader>(newCfgFile);
	auto err = m_reader->ParseError();
	if (err != 0)
	{
		LOG(ERROR) << "'" << newCfgFile.c_str() << "' parse error code:" << err;
		return SCODE_START_FAIL_EXIT_APP;
	}

	std::string appName = GApplication::getInstance()->getAppName();

	auto it = std::find(m_reader->Sections().begin(), m_reader->Sections().end(), appName);
	if (it == m_reader->Sections().end())
	{
		LOG(ERROR) << "Section '" << appName.c_str() << "' not found in '" << newCfgFile.c_str() << "'";
		return SCODE_START_FAIL_EXIT_APP;
	}

	GFileSystem::removeFile(newCfgFile);
	return SCODE_START_SUCCESS;
}

