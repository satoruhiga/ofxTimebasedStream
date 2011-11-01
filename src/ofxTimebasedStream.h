#pragma once

#include "ofMain.h"

class ofxTimebasedStream
{
public:
	
	struct Packet
	{
		float timestamp;
		size_t length;
		istream::off_type offset;
		
		inline const size_t getHeaderOffset() const { return sizeof(float) + sizeof(size_t); }
	};
	
	~ofxTimebasedStream()
	{
		close();
	}
	
	void open(string path_)
	{
		path = ofToDataPath(path_);
		
		if (ifs) ifs.close();
		if (ofs) ofs.close();
		
		ifs.open(path.c_str(), ios::in | ios::binary);
		ofs.open(path.c_str(), ios::app | ios::binary);
		
		rebuildCache();
	}
	
	void close()
	{
		if (ifs) ifs.close();
		if (ofs) ofs.close();
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
		
		ofs.seekp(0, std::ios::end);
		p.offset = ofs.tellp();
		
		ofs.write((char*)&p.timestamp, sizeof(float));
		ofs.write((char*)&length, sizeof(size_t));
		ofs.write((char*)data, length);
		
		cache.insert(pair<float, Packet>(t, p));
	}
	
	float read(float t, string &data)
	{
		const multimap<float, Packet>::const_iterator it = cache.lower_bound(t);
		const Packet &p = (*it).second;
		data.resize(p.length);
		
		ifs.clear();
		ifs.seekg(p.offset + p.getHeaderOffset(), std::ios::beg);
		ifs.read((char*)data.data(), p.length);
		
		return (*it).first;
	}
	
	float read(float t, void *data, size_t length)
	{
		const multimap<float, Packet>::const_iterator it = cache.lower_bound(t);
		const Packet &p = (*it).second;
		
		ifs.clear();
		ifs.seekg(p.offset + p.getHeaderOffset(), std::ios::beg);
		ifs.read((char*)data, length);
		
		return (*it).first;
	}
	
	bool read(float from, float to, vector<string> &data)
	{
		if (from > to) swap(from, to);
		
		const multimap<float, Packet>::const_iterator beg = cache.lower_bound(from);
		const multimap<float, Packet>::const_iterator end = cache.lower_bound(to);
		
		if (beg == end)
		{
			return false;
		}
		
		data.clear();
		ifs.clear();
		
		multimap<float, Packet>::const_iterator it = beg;
		while (it != end)
		{
			const Packet &p = (*it).second;
			string s;
			s.resize(p.length);
			
			ifs.seekg(p.offset + p.getHeaderOffset(), std::ios::beg);
			ifs.read((char*)s.data(), p.length);
			
			data.push_back(s);
			
			it++;
		}
		
		return true;
	}
	
	void optimize()
	{
		ofs.close();
		
		string temp_filename = path + ".tmp";
		ofstream temp;
		
		temp.open(temp_filename.c_str(), ios::out | ios::binary);
		
		multimap<float, Packet>::const_iterator it = cache.begin();
		while (it != cache.end())
		{
			const Packet &p = (*it).second;
			
			string data;
			data.resize(p.length);
			
			ifs.seekg(p.offset + p.getHeaderOffset(), std::ios::beg);
			ifs.read((char*)data.data(), p.length);
			
			temp.write((char*)&p.timestamp, sizeof(float));
			temp.write((char*)&p.length, sizeof(size_t));
			temp.write(data.data(), data.size());
			
			it++;
		}
		
		temp.close();
		
		ofFile::removeFile(path);
		ofFile::moveFromTo(temp_filename, path);
		
		ifs.open(path.c_str(), ios::in | ios::binary);
		ofs.open(path.c_str(), ios::app | ios::binary);
		
		rebuildCache();
	}
	
	float getPacketTimestamp(float t)
	{
		const multimap<float, Packet>::const_iterator it = cache.lower_bound(t);
		return (*it).first;
	}
	
protected:
	
	string path;
	
	multimap<float, Packet> cache;
	ifstream ifs;
	ofstream ofs;
	
	void rebuildCache()
	{
		cache.clear();
		
		ifs.seekg(0, std::ios::end);
		const size_t file_size = ifs.tellg();
		
		if (ifs.tellg() <= 0) return;
		
		ifs.clear();
		ifs.seekg(0, std::ios::beg);
		
		while (!ifs.eof())
		{
			Packet p;
			p.offset = ifs.tellg();
			
			if (file_size == p.offset) break;
			
			ifs.read((char*)&p.timestamp, sizeof(float));
			ifs.read((char*)&p.length, sizeof(size_t));
			ifs.seekg(p.length, std::ios::cur);
			
			cache.insert(pair<float, Packet>(p.timestamp, p));
		}
	}
};
