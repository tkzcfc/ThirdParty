﻿#include "GConfigService.h"
#include "Utils/GStringUtils.h"
#include "GApplication.h"
#include "../Platform/GFileSystem.h"

#define ENABEL_PRETTY_INI 1
#define ENABLE_VARIABLE_SUPPORT 1

uint32_t GConfigService::onInit()
{
	auto cfgFile = GFileSystem::getExeDirectory() + "cfg/config.ini";

	std::string cfgContent;
	if (!GFileSystem::readStringFromFile(cfgFile, cfgContent))
	{
		LogError() << "Unable to find configuration file: " << cfgFile;
		return SCODE_START_FAIL_EXIT_APP;
	}

	std::vector<std::string> lines;
	StringUtils::split(cfgContent, "\n", lines);

#if ENABEL_PRETTY_INI
	for (std::size_t i = 0; i < lines.size(); ++i)
	{
		auto line = StringUtils::trim(lines[i]);

		if (line.empty() || line[0] == ';' || line[0] == '#')
		{
			continue;
		}
		else
		{
			auto pos = line.find_first_of(";#");
			if (pos != std::string::npos)
			{
				line = StringUtils::trim(line.substr(0, pos));
			}
			lines[i] = line;
		}
	}
	cfgContent = "";
	for (auto& line : lines)
	{
		cfgContent.append("\n");
		cfgContent.append(line);
	}
#endif

#if ENABLE_VARIABLE_SUPPORT
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

		lines[i] = line;

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

	// 替换定义变量
	for (auto& kv : kvp)
	{
		cfgContent = StringUtils::replaceString(cfgContent, "${" + kv.first + "}", kv.second);
	}

	auto newCfgFile = GFileSystem::getExeDirectory() + "tmp_cfg_" + GApplication::getInstance()->getAppName() + ".ini";

	if (!GFileSystem::writeTextFile(newCfgFile, cfgContent))
	{
		LogError() << "Unable to generate temporary configuration file:: " << newCfgFile;
		return SCODE_START_FAIL_EXIT_APP;
	}

#endif
	cfgFile = newCfgFile;

	m_reader = std::make_unique<GINIReader>(cfgFile);
	auto err = m_reader->ParseError();
	if (err != 0)
	{
		LogError() << "'" << cfgFile << "' parse error code:" << err;
		return SCODE_START_FAIL_EXIT_APP;
	}

	std::string appName = GApplication::getInstance()->getAppName();

	auto it = std::find(m_reader->Sections().begin(), m_reader->Sections().end(), appName);
	if (it == m_reader->Sections().end())
	{
		LogError() << "Section '" << appName << "' not found in '" << cfgFile << "'";
		return SCODE_START_FAIL_EXIT_APP;
	}


#if ENABLE_VARIABLE_SUPPORT
	GFileSystem::removeFile(newCfgFile);
#endif
	return SCODE_START_SUCCESS;
}

