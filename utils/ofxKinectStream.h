#pragma once

#include "ofMain.h"

#include "ofxTimebasedStream.h"

namespace ofxKinectStream
{

class Recorder : public ofThread, public ofxTimebasedStream::Writer
{
public:

	Recorder()
	{
		colorImage.setUseTexture(false);
		colorImage.allocate(640, 480, OF_IMAGE_COLOR);

		depthImage.setUseTexture(false);
		depthImage.allocate(640, 480, OF_IMAGE_GRAYSCALE);

		recording = false;
	}

	~Recorder()
	{
		stop();
	}

	void start(string filename)
	{
		stop();

		filename = ofToString(filename);
		ofxTimebasedStream::Writer::open(filename);

		recordingStartTime = ofGetElapsedTimef();
		frameNum = 0;

		startThread(true, false);

		recording = true;
	}

	void stop()
	{
		recording = false;

		if (isThreadRunning())
			waitForThread();

		ofxTimebasedStream::Writer::close();
	}

	inline bool isRecording()
	{
		return recording;
	}

	inline int getFrameNum() const
	{
		return frameNum;
	}

	void addFrame(unsigned char *rgb, unsigned short *raw_depth)
	{
		if (!isRecording()) return;

		if (lock())
		{
			writeTimestamp = ofGetElapsedTimef() - recordingStartTime;

			colorImage.setFromPixels(rgb, 640, 480, OF_IMAGE_COLOR);
			depthImage.setFromPixels(raw_depth, 640, 480, OF_IMAGE_GRAYSCALE);

			if (hasNewFrame) ofLogError("ofxKinectStream::Recorder", "drop frame");
			hasNewFrame = true;

			unlock();
		}
	}

protected:

	int frameNum;

	bool hasNewFrame;
	bool recording;

	float recordingStartTime;

	float writeTimestamp;
	ofImage colorImage;
	ofShortImage depthImage;

	void threadedFunction()
	{
		while (isThreadRunning())
		{
			if (hasNewFrame && lock())
			{
				size_t len;
				ofBuffer buffer;
				ostringstream ost;

				ofSaveImage(colorImage.getPixelsRef(), buffer, OF_IMAGE_FORMAT_JPEG);
				len = buffer.size();

				ost.write((char*)&len, sizeof(size_t));
				ost << buffer;

				ofSaveImage(depthImage.getPixelsRef(), buffer, OF_IMAGE_FORMAT_TIFF);
				len = buffer.size();

				ost.write((char*)&len, sizeof(size_t));
				ost << buffer;

				write(writeTimestamp, ost.str());

				hasNewFrame = false;
				frameNum++;

				unlock();
			}

			ofSleepMillis(1);
		}
	}

};

class Player : public ofxTimebasedStream::BasePlayer
{
public:

	ofImage colorImage;
	ofShortImage depthImage;

	Player()
	{
	}

	~Player()
	{
	}

	void draw(int x, int y)
	{
		colorImage.draw(x, y);
	}

	void drawDepth(int x, int y)
	{
		depthImage.draw(x, y);
	}

protected:

	void setup()
	{
		colorImage.allocate(640, 480, OF_IMAGE_COLOR);
		depthImage.allocate(640, 480, OF_IMAGE_GRAYSCALE);
	}
	
	void onFrameNew(const string &data)
	{
		string buffer;
		ofBuffer ofbuf;
		
		istringstream ist(data);
		
		size_t len = 0;
		
		ist.read((char*)&len, sizeof(size_t));
		buffer.resize(len);
		ist.read((char*)buffer.data(), len);
		
		ofbuf.set(buffer.data(), buffer.size());
		
		ofLoadImage(colorImage.getPixelsRef(), ofbuf);
		colorImage.update();
		
		ist.read((char*)&len, sizeof(size_t));
		buffer.resize(len);
		ist.read((char*)buffer.data(), len);
		
		ofbuf.set(buffer.data(), buffer.size());
		
		ofLoadImage(depthImage.getPixelsRef(), ofbuf);
		depthImage.update();
	}
};

}