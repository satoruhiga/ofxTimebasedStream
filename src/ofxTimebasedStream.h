#pragma once

#include "ofMain.h"

#include "Poco/BinaryReader.h"
#include "Poco/BinaryWriter.h"

namespace ofxTimebasedStream
{
struct Packet
{
	float timestamp;
	size_t length;

	inline const size_t getHeaderOffset() const { return sizeof(float) + sizeof(size_t); }
};

class Writer
{
	string path;
	istream::off_type offset;

	ofstream ofs;
	ofPtr<Poco::BinaryWriter> writer;

public:

	~Writer()
	{
		close();
	}

	bool open(string path_)
	{
		close();

		path = ofToDataPath(path_);
		ofs.open(path.c_str(), ios::out | ios::binary);
		
		if (!ofs) return false;
		
		writer = ofPtr<Poco::BinaryWriter>(new Poco::BinaryWriter(ofs, Poco::BinaryWriter::NATIVE_BYTE_ORDER));

		return true;
	}

	void close()
	{
		if (writer) writer.reset();
		if (ofs) ofs.close();

		offset = 0;
	}

	void write(float t, const string &data)
	{
		write(t, data.data(), data.size());
	}

	void write(float t, const void *data, size_t length)
	{
		Packet p;
		p.timestamp = t;
		p.length = length;

		*writer << p.timestamp;
		*writer << length;
		writer->writeRaw((const char*)data, length);

		offset += p.getHeaderOffset() + length;
	}
};

class Reader
{
	string path;

	ifstream ifs;
	ofPtr<Poco::BinaryReader> reader;

	Packet p;
	istream::off_type offset;

public:

	Reader() : offset(0) {}

	~Reader()
	{
		close();
	}

	bool open(string path_)
	{
		close();

		path = ofToDataPath(path_);

		ifs.open(path.c_str(), ios::in | ios::binary);
		if (!ifs) return false;

		reader = ofPtr<Poco::BinaryReader>(new Poco::BinaryReader(ifs, Poco::BinaryReader::NATIVE_BYTE_ORDER));

		return true;
	}

	void close()
	{
		if (reader) reader.reset();
		if (ifs) ifs.close();
		
		offset = 0;
	}

	void rewind()
	{
		ifs.clear();

		ifs.seekg(0, std::ios::beg);
		offset = 0;

		*reader >> p.timestamp;
		*reader >> p.length;

		ifs.seekg(0, std::ios::beg);
	}

	inline float getTimestamp() const
	{
		return p.timestamp;
	}

	bool nextFrame(string &data)
	{
		if (isEof()) return false;

		*reader >> p.timestamp;
		*reader >> p.length;

		reader->readRaw(p.length, data);

		offset += p.getHeaderOffset() + p.length;
		return true;
	}

	inline bool isEof()
	{
		return reader->eof();
	}

	inline operator bool()
	{
		return ifs;
	}
};

}