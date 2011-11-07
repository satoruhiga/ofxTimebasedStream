#pragma once

#include "ofMain.h"

#include "ofxTimebasedStream.h"

class ofxKinectStreamRecorder : public ofThread
{
	struct KinectFrame
	{
		float timestamp;
		unsigned char rgb[640 * 480 * 3];
		unsigned short raw_depth[640 * 480 * 2];
	};
	
public:
	
	ofxKinectStreamRecorder()
	{
		recording = false;
	}
	
	~ofxKinectStreamRecorder()
	{
		stream.close();
	}
	
	void start(string filename)
	{
		stop();
		
		memset(&frame, 0, sizeof(KinectFrame));
		
		filename = ofToDataPath(filename);
		stream.open(filename);

		recordingStartTime = ofGetElapsedTimef();
		writeFrameCount = 0;
		
		startThread(true, false);
		
		recording = true;
	}
	
	void stop()
	{
		recording = false;
		
		if (isThreadRunning())
			waitForThread();
		
		stream.close();
	}
	
	bool isRecording()
	{
		return recording;
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
	bool recording;
	
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
	
	void open(string path)
	{
		path = ofToDataPath(path);
		stream.open(path);
		
		video.allocate(640, 480, OF_IMAGE_COLOR);
		depth.allocate(640, 480, OF_IMAGE_GRAYSCALE);
	}
	
	void close()
	{
		stream.close();
	}
	
	void play()
	{
		currentPlayHead = 0;
		playStartTime = ofGetElapsedTimef();
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
			float d = ofGetElapsedTimef() - playStartTime;
			float t = stream.getPacketTimestamp(d);
			
			if (frameNew)
			{
				stream.read(d, &frame, sizeof(KinectFrame));
				video.setFromPixels(frame.rgb, 640, 480, OF_IMAGE_COLOR);
				
				static unsigned char depth8[640 * 480];
				
				for (int i = 0; i < 640 * 480; i++)
				{
					depth8[i] = ofMap(frame.raw_depth[i], 0, 2048, 0, 255);
				}
				
				depth.setFromPixels(depth8, 640, 480, OF_IMAGE_GRAYSCALE);
			}
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
	ofImage depth;
	
	bool playing, frameNew;
	float lastUpdateTime, currentPlayHead;
	float playStartTime;
	float streamFrameTime;
	
};
