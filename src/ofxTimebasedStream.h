#pragma once

#include "ofMain.h"

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
		ofstream ofs;
		istream::off_type offset;
		
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
			
			return true;
		}
		
		void close()
		{
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
			
			ofs.write((char*)&p.timestamp, sizeof(float));
			ofs.write((char*)&length, sizeof(size_t));
			ofs.write((char*)data, length);
			
			offset += p.getHeaderOffset() + length;
		}
	};
	
	class Reader
	{
		string path;
		ifstream ifs;
		istream::off_type offset;
		
		Packet p;
		string data;
		
	public:
		
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
			
			rewind();
			
			return true;
		}
		
		void close()
		{
			if (ifs) ifs.close();
			offset = 0;
		}
		
		void rewind()
		{
			ifs.seekg(0, std::ios::beg);
			offset = 0;
			
			ifs.read((char*)&p.timestamp, sizeof(float));
			ifs.read((char*)&p.length, sizeof(size_t));

			ifs.seekg(0, std::ios::beg);
		}
		
		float timestamp() const
		{
			return p.timestamp;
		}
		
		float nextFrame()
		{
			ifs.read((char*)&p.timestamp, sizeof(float));
			ifs.read((char*)&p.length, sizeof(size_t));
			
			data.resize(p.length);
			ifs.read((char*)data.data(), p.length);

			offset += p.getHeaderOffset() + p.length;
			return p.timestamp;
		}
		
		void getData(string &data_)
		{
			data_ = data;
		}
		
		operator bool()
		{
			return ifs;
		}
	};
}
