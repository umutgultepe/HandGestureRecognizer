#include "StdAfx.h"
#include "SkeletonPoseDetector.h"
#ifndef SHOW_DEPTH
#define SHOW_DEPTH 1
#endif
#ifndef SHOW_BAR
#define SHOW_BAR 0
#endif


enum Handstate
{
	HAND_UNKNOWN,
	HAND_OPEN,
	HAND_CLOSED
};

class HandTracker
{

private:
	int ROIWidth;
	int ROIHeight;
	float startDistance;
	int m_Width;
	int m_Height;
	bool setUp;

	XnUserID userID;
	xn::DepthGenerator* m_DepthGenerator;
	xn::UserGenerator* m_UserGenerator;
	StartPoseDetector * m_pStartPoseDetector;
	EndPoseDetector * m_pEndPoseDetector;

public:
	HandTracker(xn::DepthGenerator* depth,xn::UserGenerator* user,StartPoseDetector * startPoseDetector,EndPoseDetector* endPoseDetector);
	~HandTracker(void);
	void initROI(XnUserID user);
	void resetROI();
	void setUserID(XnUserID user);
	bool isSetUp();
	float UpdateDepthTexture(bool m_front);
	
};

