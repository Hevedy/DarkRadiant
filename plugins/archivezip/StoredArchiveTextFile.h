#pragma once

#include "iarchive.h"
#include "stream/BinaryToTextInputStream.h"

namespace archive
{

/// \brief An ArchiveTextFile which is stored uncompressed as part of a larger archive file.
class StoredArchiveTextFile :
	public ArchiveTextFile
{
private:
	std::string _name;
	stream::FileInputStream _filestream;
	stream::SubFileInputStream _substream; // provides a subset of _filestream
	stream::BinaryToTextInputStream<stream::SubFileInputStream> _textStream; // converts data from _substream

	// Mod directory
	std::string _modName;
public:
	typedef stream::FileInputStream::size_type size_type;
	typedef stream::FileInputStream::position_type position_type;

	/**
	* Constructor.
	*
	* @param modDir
	* Name of the mod directory containing this file.
	*/
	StoredArchiveTextFile(const std::string& name,
						  const std::string& archiveName,
						  const std::string& modName,
						  position_type position,
						  size_type stream_size) : 
		_name(name),
		_filestream(archiveName),
		_substream(_filestream, position, stream_size),
		_textStream(_substream),
		_modName(modName)
	{}

	const std::string& getName() const override
	{
		return _name;
	}

	TextInputStream& getInputStream() override
	{
		return _textStream;
	}

	/**
	* Return mod directory.
	*/
	std::string getModName() const override
	{
		return _modName;
	}
};

}
