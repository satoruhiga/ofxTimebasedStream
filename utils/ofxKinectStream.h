#pragma once

#include "ofMain.h"

#include "ofxTimebasedStream.h"

class ofxKinectStreamRecorder : ofThread
{
	struct KinectFrame
	{
		float timestamp;
		unsigned char rgb[640 * 480 * 3];
		unsigned short raw_depth[640 * 480 * 2];
	};
	
public:
	
	void open(string filename)
	{
		memset(&frame, 0, sizeof(KinectFrame));
		
		filename = ofToDataPath(filename);
		stream.open(filename);
	}
	
	void close()
	{
		waitForThread();
	}
	
	void start()
	{
		stop();
		
		recordingStartTime = ofGetElapsedTimef();
		writeFrameCount = 0;
		
		startThread(true, false);
	}
	
	void stop()
	{
		if (isThreadRunning())
		{
			stopThread();
		}
	}
	
	void setRecording(bool yn)
	{
		if (yn) start();
		else stop();
	}
	
	bool isRecording()
	{
		return isThreadRunning();
	}
	
	void addFrame(unsigned char *rgb, unsigned short *raw_depth)
	{
		if (!isRecording()) return;
		
		if (lock())
		{
			memcpy(frame.rgb, rgb, 640 * 480 * 3);
			memcpy(frame.raw_depth, raw_depth, 640 * 480 * 2);
			frame.timestamp = ofGetElapsedTimef() - recordingStartTime;
			
			hasNewFrame = true;
			
			unlock();
		}
	}
	
protected:
	
	int writeFrameCount;
	
	bool hasNewFrame;
	KinectFrame frame;
	
	float recordingStartTime;
	ofxTimebasedStream stream;
	
	void threadedFunction()
	{
		while (isThreadRunning())
		{
			if (hasNewFrame && lock())
			{
				const size_t frame_size = (640 * 480 * 3) + (640 * 480 * 2);
				stream.write(frame.timestamp, &frame.timestamp, frame_size);
				
				hasNewFrame = false;
				writeFrameCount++;
				
				unlock();
			}
			
			ofSleepMillis(10);
		}
	}
};


class ofxKinectStreamPlayer
{
	struct KinectFrame
	{
		float timestamp;
		unsigned char rgb[640 * 480 * 3];
		unsigned short raw_depth[640 * 480 * 2];
	};
	
public:
	
	void init()
	{
		video.allocate(640, 480, OF_IMAGE_COLOR);
		depth.allocate(640, 480, OF_IMAGE_GRAYSCALE);
	}
	
	void open(string path)
	{
		path = ofToDataPath(path);
		stream.open(path);
	}
	
	void close()
	{
		stream.close();
	}
	
	void play()
	{
		currentPlayHead = 0;
		playing = true;
	}
	
	void stop()
	{
		playing = false;
	}
	
	bool isFrameNew()
	{
		return frameNew;
	}
	
	void update()
	{
		if (playing)
		{
			float d = ofGetElapsedTimef() - lastUpdateTime;
			currentPlayHead += d;
			
			float t = stream.getPacketTimestamp(currentPlayHead);
			frameNew = t != streamFrameTime;
			streamFrameTime = t;
			
			if (frameNew)
			{
				stream.read(currentPlayHead, &frame, sizeof(KinectFrame));
				video.setFromPixels(frame.rgb, 640, 480, OF_IMAGE_COLOR);
				depth.setFromPixels(frame.raw_depth, 640, 480, OF_IMAGE_GRAYSCALE);
			}
			
			lastUpdateTime = ofGetElapsedTimef();
		}
	}
	
	void draw(int x, int y)
	{
		video.draw(x, y);
	}
	
	void drawDepth(int x, int y)
	{
		depth.draw(x, y);
	}
	
private:
	
	ofxTimebasedStream stream;
	KinectFrame frame;
	
	ofImage video;
	ofShortImage depth;
	
	bool playing, frameNew;
	float lastUpdateTime, currentPlayHead;
	float streamFrameTime;
	
};
