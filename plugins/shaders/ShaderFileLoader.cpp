#include "ShaderFileLoader.h"

#include "ifilesystem.h"
#include "iarchive.h"
#include "i18n.h"
#include "parser/DefTokeniser.h"
#include "parser/DefBlockTokeniser.h"
#include "ShaderDefinition.h"
#include "Doom3ShaderSystem.h"
#include "TableDefinition.h"

#include <iostream>
#include "string/replace.h"

/* FORWARD DECLS */

namespace shaders
{

/* Parses through the shader file and processes the tokens delivered by
 * DefTokeniser.
 */
void ShaderFileLoader::parseShaderFile(std::istream& inStr,
									   const std::string& filename)
{
	// Parse the file with a blocktokeniser, the actual block contents
	// will be parsed separately.
	parser::BasicDefBlockTokeniser<std::istream> tokeniser(inStr);

	while (tokeniser.hasMoreBlocks())
	{
		// Get the next block
		parser::BlockTokeniser::Block block = tokeniser.nextBlock();

		// Skip tables
		if (block.name.substr(0, 5) == "table")
		{
			std::string tableName = block.name.substr(6);

			if (tableName.empty())
			{
				rError() << "[shaders] " << filename << ": Missing table name." << std::endl;
				continue;
			}

			TableDefinitionPtr table(new TableDefinition(tableName, block.contents));

			if (!_library.addTableDefinition(table))
			{
				rError() << "[shaders] " << filename
					<< ": table " << tableName << " already defined." << std::endl;
			}

			continue;
		}
		else if (block.name.substr(0, 5) == "skin ")
		{
			continue; // skip skin definition
		}
		else if (block.name.substr(0, 9) == "particle ")
		{
			continue; // skip particle definition
		}

		string::replace_all(block.name, "\\", "/"); // use forward slashes

		ShaderTemplatePtr shaderTemplate(new ShaderTemplate(block.name, block.contents));

		// Construct the ShaderDefinition wrapper class
		ShaderDefinition def(shaderTemplate, filename);

		// Insert into the definitions map, if not already present
		if (!_library.addDefinition(block.name, def))
		{
    		rError() << "[shaders] " << filename
				<< ": shader " << block.name << " already defined." << std::endl;
		}
	}
}

void ShaderFileLoader::addFile(const std::string& filename)
{
	// Construct the full VFS path
	_files.push_back(_basePath + filename);
}

void ShaderFileLoader::parseFiles()
{
	for (std::size_t i = 0; i < _files.size(); ++i)
	{
		const std::string& fullPath = _files[i];

		if (_currentOperation)
		{
			_currentOperation->setMessage(fmt::format(_("Parsing material file {0}"), fullPath));

			float progress = static_cast<float>(i) / _files.size();
			_currentOperation->setProgress(progress);
		}

		// Open the file
		ArchiveTextFilePtr file = GlobalFileSystem().openTextFile(fullPath);

		if (file != NULL)
		{
			std::istream is(&(file->getInputStream()));
			parseShaderFile(is, fullPath);
		}
		else
		{
			throw std::runtime_error("Unable to read shaderfile: " + fullPath);
		}
	}
}

} // namespace shaders
