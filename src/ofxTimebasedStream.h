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

	virtual ~Writer()
	{
		close();
	}

	bool open(string path_)
	{
		close();

		path = ofToDataPath(path_);
		ofs.open(path.c_str(), ios::out | ios::binary);

		offset = 0;
		
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

public:

	Reader()
	{
		p.timestamp = 0;
		p.length = 0;
	}

	virtual ~Reader()
	{
		close();
	}

	bool load(string path_)
	{
		close();

		path = ofToDataPath(path_);
		ifs.open(path.c_str(), ios::in | ios::binary);

		offset = 0;
		
		if (!ifs) return false;

		rewind();

		return true;
	}
	
	inline bool open(string path) { return load(path); } // alias

	void close()
	{
		if (ifs) ifs.close();
		offset = 0;
	}

	void rewind()
	{
		ifs.clear();

		ifs.seekg(0, std::ios::beg);
		offset = 0;

		ifs.read((char*)&p.timestamp, sizeof(float));
		ifs.read((char*)&p.length, sizeof(size_t));

		ifs.seekg(0, std::ios::beg);
	}

	inline float getTimestamp() const
	{
		return p.timestamp;
	}

	bool nextFrame(string &data)
	{
		if (isEof()) return false;

		ifs.read((char*)&p.timestamp, sizeof(float));
		ifs.read((char*)&p.length, sizeof(size_t));

		data.resize(p.length);
		ifs.read((char*)data.data(), p.length);

		offset += p.getHeaderOffset() + p.length;
		return true;
	}

protected:
	
	inline bool isEof()
	{
		return ifs.eof();
	}

	operator bool()
	{
		return ifs;
	}
};

class BasePlayer : public Reader
{
public:

	BasePlayer() : frameNum(0), rate(1), playing(false), frameNew(false), loop(false), needUpdate(true) {}

	bool load(string path)
	{
		if (!ofxTimebasedStream::Reader::load(path)) return false;

		if (*this)
		{
			setup();
			rewind();
			
			return true;
		}
		
		return false;
	}
	
	inline bool open(string path) { return load(path); }

	virtual ~BasePlayer()
	{
		close();
	}

	void play()
	{
		rate = 1;
		playing = true;
		frameNew = false;
	}

	void stop()
	{
		rate = 0;
		playing = false;
		frameNew = false;
	}

	inline bool isPlaying() const
	{
		return playing;
	}

	void rewind()
	{
		ofxTimebasedStream::Reader::rewind();

		playHeadTime = 0;
		frameNum = 0;
		frameNew = false;
		
		needUpdate = true;
	}

	void update()
	{
		frameNew = false;

		if (playing)
		{
			setPlayHeadTime(playHeadTime + ofGetLastFrameTime() * rate);
		}
		
		if (needUpdate)
		{
			needUpdate = false;
			
			string data;
			
			// skip to seek head
			while (playHeadTime > getTimestamp())
			{
				if (!nextFrame(data))
				{
					if (isLoop())
					{
						rewind();
					}
					else
					{
						playing = false;
						rewind();
					}
					
					data.clear();
					
					break;
				}
			}
			
			if (!data.empty())
			{
				onFrameNew(data);
				
				frameNum++;
				frameNew = true;
			}
		}
	}

	inline bool isFrameNew() const { return frameNew; }
	inline int getFrameNum() const { return frameNum; }

	inline bool isLoop() const { return loop; }
	inline void setLoop(bool yn) { loop = yn; }

	inline float getPlayHeadTime() const { return playHeadTime; }
	inline void setPlayHeadTime(float t)
	{
		if (t == 0 || t < playHeadTime)
			rewind();
		
		playHeadTime = t;
		
		needUpdate = true;
	}

	inline float getRate() const { return rate; }
	inline void setRate(float v) { rate = v; }

protected:

	virtual void setup() = 0;
	virtual void onFrameNew(const string &data) = 0;

private:

	bool playing, frameNew;
	float playHeadTime;
	int frameNum;
	bool loop;
	float rate;
	bool needUpdate;

};

}