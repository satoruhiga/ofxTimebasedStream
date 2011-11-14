#pragma once

#include "ofMain.h"

#include "ofxTimebasedStream.h"

namespace ofxKinectStream
{
	
	class Recorder : public ofThread
	{
	public:
		
		Recorder()
		{
			colorImage.setUseTexture(false);
			colorImage.allocate(640, 480, OF_IMAGE_COLOR);
			
			depthImage.setUseTexture(false);
			depthImage.allocate(640, 470, OF_IMAGE_GRAYSCALE);
		}
		
		~Recorder()
		{
			stop();
		}
		
		void start(string filename)
		{
			stop();
			
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
		
		int getWriteFrameCount() const { return writeFrameCount; }
		
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
		
		int writeFrameCount;
		
		bool hasNewFrame;
		bool recording;
		
		float recordingStartTime;
		ofxTimebasedStream::Writer stream;
		
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

					stream.write(writeTimestamp, ost.str());
					
					hasNewFrame = false;
					writeFrameCount++;
					
					unlock();
				}
				
				ofSleepMillis(1);
			}
		}
		
	};
	
	
	class Player
	{
	public:
		
		~Player()
		{
			close();
		}
		
		void open(string path)
		{
			path = ofToDataPath(path);
			stream.open(path);
			
			if (stream)
			{
				colorImage.allocate(640, 480, OF_IMAGE_COLOR);
				depthImage.allocate(640, 480, OF_IMAGE_GRAYSCALE);
			}
		}
		
		void close()
		{
			stream.close();
		}
		
		void play()
		{
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
		
		float getPosition()
		{
			return stream.timestamp();
		}
		
		void update()
		{
			if (stream && playing)
			{
				float d = ofGetElapsedTimef() - playStartTime;
				
				while (d > stream.timestamp())
				{
					// skip to seek head
					stream.nextFrame();
				}
				
				string data;
				string buffer;
				ofBuffer ofbuf;
				stream.getData(data);
				
				istringstream ist(data);
				
				size_t len;
				
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
		}
		
		void draw(int x, int y)
		{
			colorImage.draw(x, y);
		}
		
		void drawDepth(int x, int y)
		{
			depthImage.draw(x, y);
		}
		
	private:
		
		ofxTimebasedStream::Reader stream;
		
		ofImage colorImage;
		ofShortImage depthImage;
		
		bool playing, frameNew;
		float playStartTime;
		
	};
	
}

/*
*/