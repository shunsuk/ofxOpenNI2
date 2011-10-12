/*
 * ofxOpenNI.cpp
 *
 *  Created on: 11/10/2011
 *      Author: arturo
 */


#include "ofxOpenNI.h"

#include <XnLog.h>

#include "ofxOpenNIUtils.h"

#include "ofLog.h"


string ofxOpenNI::LOG_NAME = "ofxOpenNI";

using namespace xn;

static bool rainbowPalletInit = false;
XnUInt8 PalletIntsR [256] = {0};
XnUInt8 PalletIntsG [256] = {0};
XnUInt8 PalletIntsB [256] = {0};

static void CreateRainbowPallet() {
	if(rainbowPalletInit) return;

	unsigned char r, g, b;
	for (int i=1; i<255; i++) {
		if (i<=29) {
			r = (unsigned char)(129.36-i*4.36);
			g = 0;
			b = (unsigned char)255;
		}
		else if (i<=86) {
			r = 0;
			g = (unsigned char)(-133.54+i*4.52);
			b = (unsigned char)255;
		}
		else if (i<=141) {
			r = 0;
			g = (unsigned char)255;
			b = (unsigned char)(665.83-i*4.72);
		}
		else if (i<=199) {
			r = (unsigned char)(-635.26+i*4.47);
			g = (unsigned char)255;
			b = 0;
		}
		else {
			r = (unsigned char)255;
			g = (unsigned char)(1166.81-i*4.57);
			b = 0;
		}
		PalletIntsR[i] = r;
		PalletIntsG[i] = g;
		PalletIntsB[i] = b;
	}
	rainbowPalletInit = true;
}

ofxOpenNI::ofxOpenNI(){
	CreateRainbowPallet();

	g_bIsDepthOn = false;
	g_bIsImageOn = false;
	g_bIsIROn = false;
	g_bIsAudioOn = false;
	g_bIsPlayerOn = false;

	g_pPrimary = NULL;

	depth_coloring = COLORING_RAINBOW;

	useTexture = true;
}

void ofxOpenNI::initConstants(){
	// Primary Streams
	int nIndex = 0;

	g_PrimaryStream.pValues[nIndex++] = "Any";
	g_PrimaryStream.pValues[nIndex++] = xnProductionNodeTypeToString(XN_NODE_TYPE_DEPTH);
	g_PrimaryStream.pValues[nIndex++] = xnProductionNodeTypeToString(XN_NODE_TYPE_IMAGE);
	g_PrimaryStream.pValues[nIndex++] = xnProductionNodeTypeToString(XN_NODE_TYPE_IR);
	g_PrimaryStream.pValues[nIndex++] = xnProductionNodeTypeToString(XN_NODE_TYPE_AUDIO);

	g_PrimaryStream.nValuesCount = nIndex;

	// Registration
	nIndex = 0;

	g_Registration.pValues[nIndex++] = FALSE;
	g_Registration.pValueToName[FALSE] = "Off";

	g_Registration.pValues[nIndex++] = TRUE;
	g_Registration.pValueToName[TRUE] = "Depth -> Image";

	g_Registration.nValuesCount = nIndex;

	// Resolutions
	nIndex = 0;

	g_Resolution.pValues[nIndex++] = XN_RES_QVGA;
	g_Resolution.pValueToName[XN_RES_QVGA] = Resolution(XN_RES_QVGA).GetName();

	g_Resolution.pValues[nIndex++] = XN_RES_VGA;
	g_Resolution.pValueToName[XN_RES_VGA] = Resolution(XN_RES_VGA).GetName();

	g_Resolution.pValues[nIndex++] = XN_RES_SXGA;
	g_Resolution.pValueToName[XN_RES_SXGA] = Resolution(XN_RES_SXGA).GetName();

	g_Resolution.pValues[nIndex++] = XN_RES_UXGA;
	g_Resolution.pValueToName[XN_RES_UXGA] = Resolution(XN_RES_UXGA).GetName();

	g_Resolution.nValuesCount = nIndex;

}


void ofxOpenNI::allocateDepthBuffers(){
	if(g_bIsDepthOn){
		max_depth	= g_Depth.GetDeviceMaxDepth();
		depthPixels[0].allocate(640,480,OF_IMAGE_COLOR_ALPHA);
		depthPixels[1].allocate(640,480,OF_IMAGE_COLOR_ALPHA);
		currentDepthPixels = &depthPixels[0];
		backDepthPixels = &depthPixels[1];
		if(useTexture) depthTexture.allocate(640,480,GL_RGBA);
	}
}

void ofxOpenNI::allocateRGBBuffers(){
	if(g_bIsImageOn){
		rgbPixels[0].allocate(640,480,OF_IMAGE_COLOR);
		rgbPixels[1].allocate(640,480,OF_IMAGE_COLOR);
		currentRGBPixels = &rgbPixels[0];
		backRGBPixels = &rgbPixels[1];
		if(useTexture) rgbTexture.allocate(640,480,GL_RGB);
	}
}

void XN_CALLBACK_TYPE ofxOpenNI::onErrorStateChanged(XnStatus errorState, void* pCookie){

	if (errorState != XN_STATUS_OK){
		ofLogError(LOG_NAME) << xnGetStatusString(errorState);
		//setErrorState(xnGetStatusString(errorState));
	}else{
		//setErrorState(NULL);
	}
}

void ofxOpenNI::openCommon(){
	XnStatus nRetVal = XN_STATUS_OK;

	g_bIsDepthOn = false;
	g_bIsImageOn = false;
	g_bIsIROn = false;
	g_bIsAudioOn = false;
	g_bIsPlayerOn = false;

	NodeInfoList list;
	nRetVal = g_Context.EnumerateExistingNodes(list);
	if (nRetVal == XN_STATUS_OK)
	{
		for (NodeInfoList::Iterator it = list.Begin(); it != list.End(); ++it)
		{
			switch ((*it).GetDescription().Type)
			{
			case XN_NODE_TYPE_DEVICE:
				ofLogVerbose(LOG_NAME) << "Creating device";
				(*it).GetInstance(g_Device);
				break;
			case XN_NODE_TYPE_DEPTH:
				ofLogVerbose(LOG_NAME) << "Creating depth generator";
				g_bIsDepthOn = true;
				(*it).GetInstance(g_Depth);
				break;
			case XN_NODE_TYPE_IMAGE:
				ofLogVerbose(LOG_NAME) << "Creating image generator";
				g_bIsImageOn = true;
				(*it).GetInstance(g_Image);
				break;
			case XN_NODE_TYPE_IR:
				ofLogVerbose(LOG_NAME) << "Creating ir generator";
				g_bIsIROn = true;
				(*it).GetInstance(g_IR);
				break;
			case XN_NODE_TYPE_AUDIO:
				ofLogVerbose(LOG_NAME) << "Creating audio generator";
				g_bIsAudioOn = true;
				(*it).GetInstance(g_Audio);
				break;
			case XN_NODE_TYPE_PLAYER:
				ofLogVerbose(LOG_NAME) << "Creating player";
				g_bIsPlayerOn = true;
				(*it).GetInstance(g_Player);
				break;
			}
		}
	}

	XnCallbackHandle hDummy;
	g_Context.RegisterToErrorStateChange(onErrorStateChanged, this, hDummy);

	initConstants();
	allocateDepthBuffers();
	allocateRGBBuffers();

	readFrame();
}

void ofxOpenNI::addLicense(string sVendor, string sKey) {

	XnLicense license = {0};
	XnStatus status = XN_STATUS_OK;

	status = xnOSStrNCopy(license.strVendor, sVendor.c_str(),sVendor.size(), sizeof(license.strVendor));
	if(status != XN_STATUS_OK) {
		ofLogError(LOG_NAME) << "ofxOpenNIContext error creating license (vendor)";
		return;
	}

	status = xnOSStrNCopy(license.strKey, sKey.c_str(), sKey.size(), sizeof(license.strKey));
	if(status != XN_STATUS_OK) {
		ofLogError(LOG_NAME) << "ofxOpenNIContext error creating license (key)";
		return;
	}

	status = g_Context.AddLicense(license);
	SHOW_RC(status, "AddLicense");

	xnPrintRegisteredLicenses();

}

bool ofxOpenNI::setupFromXML(string xml, bool color, bool depth, bool IR){
	XnStatus nRetVal = XN_STATUS_OK;
	EnumerationErrors errors;

	nRetVal = g_Context.InitFromXmlFile(ofToDataPath(xml).c_str(), g_scriptNode, &errors);
	SHOW_RC(nRetVal, "setup from XML");

	if(nRetVal!=XN_STATUS_OK) return false;

	openCommon();

	startThread(true,false);

	return true;
}

bool ofxOpenNI::setupFromRecording(string recording){
	xnLogInitFromXmlFile(ofToDataPath("openni/config/ofxopenni_config.xml").c_str());

	XnStatus nRetVal = g_Context.Init();
	if(nRetVal!=XN_STATUS_OK) return false;

	addLicense("PrimeSense", "0KOIk2JeIBYClPWVnMoRKn5cdY4=");

	nRetVal = g_Context.OpenFileRecording(ofToDataPath(recording).c_str(), g_Player);
	SHOW_RC(nRetVal, "setup from recording");

	if(nRetVal!=XN_STATUS_OK) return false;

	openCommon();

	startThread(true,false);

	return true;
}

void ofxOpenNI::setDepthColoring(DepthColoring coloring){
	depth_coloring = coloring;
}

void ofxOpenNI::readFrame(){
	XnStatus rc = XN_STATUS_OK;

	if (g_pPrimary != NULL){
		rc = g_Context.WaitOneUpdateAll(*g_pPrimary);
	}else{
		rc = g_Context.WaitAnyUpdateAll();
	}

	if (rc != XN_STATUS_OK){
		ofLogError(LOG_NAME) << "Error:" << xnGetStatusString(rc);
	}

	if (g_Depth.IsValid()){
		g_Depth.GetMetaData(g_DepthMD);
	}

	if (g_Image.IsValid()){
		g_Image.GetMetaData(g_ImageMD);
	}

	if (g_IR.IsValid()){
		g_IR.GetMetaData(g_irMD);
	}

	if (g_Audio.IsValid()){
		g_Audio.GetMetaData(g_AudioMD);
	}
}

void ofxOpenNI::threadedFunction(){
	while(isThreadRunning()){
		readFrame();
		if(g_bIsDepthOn){
			generateDepthPixels();
		}
		if(g_bIsImageOn){
			generateImagePixels();
		}
		lock();
		if(g_bIsDepthOn){
			ofPixels * auxP = backDepthPixels;
			backDepthPixels = currentDepthPixels;
			currentDepthPixels = auxP;
		}
		if(g_bIsImageOn){
			ofPixels * auxP = backRGBPixels;
			backRGBPixels = currentRGBPixels;
			currentRGBPixels = auxP;
		}
		bNewPixels = true;
		unlock();
	}
}

bool ofxOpenNI::isNewFrame(){
	return bNewFrame;
}

void ofxOpenNI::update(){
	lock();
	if(bNewPixels){
		if(g_bIsDepthOn && useTexture){
			depthTexture.loadData(*currentDepthPixels);
		}
		if(g_bIsImageOn && useTexture){
			rgbTexture.loadData(*currentRGBPixels);
		}
		bNewPixels = false;
		bNewFrame = true;
	}
	unlock();
}

void ofxOpenNI::generateImagePixels(){
	xn::ImageMetaData imd;
	g_Image.GetMetaData(imd);
	const XnUInt8* pImage = imd.Data();

	backRGBPixels->setFromPixels(pImage, imd.XRes(),imd.YRes(),OF_IMAGE_COLOR);
}

void ofxOpenNI::draw(int x, int y){
	depthTexture.draw(x,y);
}


void ofxOpenNI::drawRGB(int x, int y){
	rgbTexture.draw(x,y);
}

void ofxOpenNI::generateDepthPixels(){

	// get the pixels
	const XnDepthPixel* depth = g_DepthMD.Data();
	XN_ASSERT(depth);

	if (g_DepthMD.FrameID() == 0) return;

	// copy depth into texture-map
	float max;
	for (XnUInt16 y = g_DepthMD.YOffset(); y < g_DepthMD.YRes() + g_DepthMD.YOffset(); y++) {
		unsigned char * texture = backDepthPixels->getPixels() + y * g_DepthMD.XRes() * 4 + g_DepthMD.XOffset() * 4;
		for (XnUInt16 x = 0; x < g_DepthMD.XRes(); x++, depth++, texture += 4) {
			XnUInt8 red = 0;
			XnUInt8 green = 0;
			XnUInt8 blue = 0;
			XnUInt8 alpha = 255;

			XnUInt16 col_index;

			switch (depth_coloring){
				case COLORING_PSYCHEDELIC_SHADES:
					alpha *= (((XnFloat)(*depth % 10) / 20) + 0.5);
				case COLORING_PSYCHEDELIC:
					switch ((*depth/10) % 10){
						case 0:
							red = 255;
							break;
						case 1:
							green = 255;
							break;
						case 2:
							blue = 255;
							break;
						case 3:
							red = 255;
							green = 255;
							break;
						case 4:
							green = 255;
							blue = 255;
							break;
						case 5:
							red = 255;
							blue = 255;
							break;
						case 6:
							red = 255;
							green = 255;
							blue = 255;
							break;
						case 7:
							red = 127;
							blue = 255;
							break;
						case 8:
							red = 255;
							blue = 127;
							break;
						case 9:
							red = 127;
							green = 255;
							break;
					}
					break;
				case COLORING_RAINBOW:
					col_index = (XnUInt16)(((*depth) / (max_depth / 256)));
					red = PalletIntsR[col_index];
					green = PalletIntsG[col_index];
					blue = PalletIntsB[col_index];
					break;
				case COLORING_CYCLIC_RAINBOW:
					col_index = (*depth % 256);
					red = PalletIntsR[col_index];
					green = PalletIntsG[col_index];
					blue = PalletIntsB[col_index];
					break;
				case COLORING_BLUES:
					// 3 bytes of depth: black (R0G0B0) >> blue (001) >> cyan (011) >> white (111)
					max = 256+255+255;
					col_index = (XnUInt16)(((*depth) / ( max_depth / max)));
					if ( col_index < 256 )
					{
						blue	= col_index;
						green	= 0;
						red		= 0;
					}
					else if ( col_index < (256+255) )
					{
						blue	= 255;
						green	= (col_index % 256) + 1;
						red		= 0;
					}
					else if ( col_index < (256+255+255) )
					{
						blue	= 255;
						green	= 255;
						red		= (col_index % 256) + 1;
					}
					else
					{
						blue	= 255;
						green	= 255;
						red		= 255;
					}
					break;
				case COLORING_GREY:
					max = 255;	// half depth
					{
						XnUInt8 a = (XnUInt8)(((*depth) / ( max_depth / max)));
						red		= a;
						green	= a;
						blue	= a;
					}
					break;
				case COLORING_STATUS:
					// This is something to use on installations
					// when the end user needs to know if the camera is tracking or not
					// The scene will be painted GREEN if status == true
					// The scene will be painted RED if status == false
					// Usage: declare a global bool status and that's it!
					// I'll keep it commented so you dont have to have a status on every project
#if 0
					{
						extern bool status;
						max = 255;	// half depth
						XnUInt8 a = 255 - (XnUInt8)(((*depth) / ( max_depth / max)));
						red		= ( status ? 0 : a);
						green	= ( status ? a : 0);
						blue	= 0;
					}
#endif
					break;
			}

			texture[0] = red;
			texture[1] = green;
			texture[2] = blue;

			if (*depth == 0)
				texture[3] = 0;
			else
				texture[3] = alpha;
		}
	}

}

xn::Context & ofxOpenNI::getXnContext(){
	return g_Context;
}

xn::Device & ofxOpenNI::getDevice(){
	return g_Device;
}

xn::DepthGenerator & ofxOpenNI::getDepthGenerator(){
	return g_Depth;
}

xn::ImageGenerator & ofxOpenNI::getImageGenerator(){
	return g_Image;
}

xn::IRGenerator & ofxOpenNI::getIRGenerator(){
	return g_IR;
}

xn::AudioGenerator & ofxOpenNI::getAudioGenerator(){
	return g_Audio;
}

xn::Player & ofxOpenNI::getPlayer(){
	return g_Player;
}

xn::DepthMetaData & ofxOpenNI::getDepthMetaData(){
	return g_DepthMD;
}

xn::ImageMetaData & ofxOpenNI::getImageMetaData(){
	return g_ImageMD;
}

xn::IRMetaData & ofxOpenNI::getIRMetaData(){
	return g_irMD;
}

xn::AudioMetaData & ofxOpenNI::getAudioMetaData(){
	return g_AudioMD;
}

ofPixels & ofxOpenNI::getDepthPixels(){
	Poco::ScopedLock<ofMutex> lock(mutex);
	return *currentDepthPixels;
}

ofPixels & ofxOpenNI::getRGBPixels(){
	Poco::ScopedLock<ofMutex> lock(mutex);
	return *currentRGBPixels;
}

float ofxOpenNI::getWidth(){
	if(g_bIsDepthOn){
		return g_DepthMD.XRes();
	}else if(g_bIsImageOn){
		return g_ImageMD.XRes();
	}else if(g_bIsIROn){
		return g_irMD.XRes();
	}else{
		return 0;
	}
}

float ofxOpenNI::getHeight(){
	if(g_bIsDepthOn){
		return g_DepthMD.YRes();
	}else if(g_bIsImageOn){
		return g_ImageMD.YRes();
	}else if(g_bIsIROn){
		return g_irMD.YRes();
	}else{
		return 0;
	}
}
