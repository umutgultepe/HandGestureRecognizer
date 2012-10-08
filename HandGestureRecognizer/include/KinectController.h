/*****************************************************************************
*                                                                            *
*  Sinbad Sample Application                                                 *
*  Copyright (C) 2010 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of OpenNI.                                              *
*                                                                            *
*  OpenNI is free software: you can redistribute it and/or modify            *
*  it under the terms of the GNU Lesser General Public License as published  *
*  by the Free Software Foundation, either version 3 of the License, or      *
*  (at your option) any later version.                                       *
*                                                                            *
*  OpenNI is distributed in the hope that it will be useful,                 *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              *
*  GNU Lesser General Public License for more details.                       *
*                                                                            *
*  You should have received a copy of the GNU Lesser General Public License  *
*  along with OpenNI. If not, see <http://www.gnu.org/licenses/>.            *
*                                                                            *
*****************************************************************************/

// This sample is based on the Character sample from the OgreSDK.

#ifndef __Sinbad_H__
#define __Sinbad_H__
#include "StdAfx.h"
#ifndef SHOW_DEPTH
#define SHOW_DEPTH 1
#endif
#ifndef SHOW_BAR
#define SHOW_BAR 0
#endif


#include "SkeletonPoseDetector.h"
#include "HandTracker.h"


#if SHOW_DEPTH && DEPTH_BAR
#	error SHOW_DEPTH and SHOW_BAR are mutually exclusive
#endif 



	class YesNoSlider : public OgreBites::Widget
	{
	public:

		// Do not instantiate any widgets directly. Use SdkTrayManager.
		YesNoSlider(const Ogre::String& name, const Ogre::DisplayString& caption, Ogre::Real width, Ogre::Real trackWidth,
			 Ogre::Real minValue, Ogre::Real maxValue, unsigned int snaps)
			: mDragOffset(0.0f)
			, mValue(0.0f)
			, mMinValue(0.0f)
			, mMaxValue(0.0f)
			, mInterval(0.0f)
		{
			mDragging = false;
			mFitToContents = false;
			mElement = Ogre::OverlayManager::getSingleton().createOverlayElementFromTemplate
				("SdkTrays/YesNoSlider", "BorderPanel", name);
			mElement->setWidth(width);
			
			Ogre::OverlayContainer* c = (Ogre::OverlayContainer*)mElement;
			mTextArea = (Ogre::TextAreaOverlayElement*)c->getChild(getName() + "/YesNoSliderCaption");
			
			mYesArea = (Ogre::TextAreaOverlayElement*)c->getChild(getName() + "/YesText");
			mNoArea = (Ogre::TextAreaOverlayElement*)c->getChild(getName() + "/NoText");
			
			mTrack = (Ogre::BorderPanelOverlayElement*)c->getChild(getName() + "/YesNoSliderTrack");
			mHandle = (Ogre::PanelOverlayElement*)mTrack->getChild(mTrack->getName() + "/YesNoSliderHandle");
#if OGRE_PLATFORM == OGRE_PLATFORM_IPHONE
            mTextArea->setCharHeight(mTextArea->getCharHeight() - 3);
            
#endif
            
			mTrack->setWidth(trackWidth);
			mTrack->setHorizontalAlignment(Ogre::GHA_CENTER);
			mTrack->setLeft(-(mTrack->getWidth()/2));
			

			setCaption(caption);
			setRange(minValue, maxValue, snaps, false);
		}

		/*-----------------------------------------------------------------------------
		| Sets the minimum value, maximum value, and the number of snapping points.
		-----------------------------------------------------------------------------*/
		void setRange(Ogre::Real minValue, Ogre::Real maxValue, unsigned int snaps, bool notifyListener = true)
		{
			mMinValue = minValue;
			mMaxValue = maxValue;
		}

		void setValue(Ogre::Real value, bool notifyListener = true)
		{
			mValue = Ogre::Math::Clamp<Ogre::Real>(value, mMinValue, mMaxValue);

			mHandle->setLeft((int)((mValue - mMinValue) / (mMaxValue - mMinValue) *
				(mTrack->getWidth() - mHandle->getWidth())));
		}

		Ogre::Real getValue()
		{
			return mValue;
		}

		const Ogre::DisplayString& getCaption()
		{
			return mTextArea->getCaption();
		}

		void setCaption(const Ogre::DisplayString& caption)
		{
			mTextArea->setCaption(caption);
			mTextArea->setLeft(-getCaptionWidth(caption, mTextArea)/2);

			//mElement->setHeight(mTextArea->getHeight() + mTrack->getHeight() + 10);
				  
		}

		void _cursorPressed(const Ogre::Vector2& cursorPos)
		{
			if (!mHandle->isVisible()) return;

			Ogre::Vector2 co = Widget::cursorOffset(mHandle, cursorPos);

			if (co.squaredLength() <= 81)
			{
				mDragging = true;
				mDragOffset = co.x;
			}
			else if (Widget::isCursorOver(mTrack, cursorPos))
			{
				Ogre::Real newLeft = mHandle->getLeft() + co.x;
				Ogre::Real rightBoundary = mTrack->getWidth() - mHandle->getWidth();

				mHandle->setLeft(Ogre::Math::Clamp<int>((int)newLeft, 0, (int)rightBoundary));
				setValue(getSnappedValue(newLeft / rightBoundary));
			}
		}

		void _cursorReleased(const Ogre::Vector2& cursorPos)
		{
			if (mDragging)
			{
				mDragging = false;
				mHandle->setLeft((int)((mValue - mMinValue) / (mMaxValue - mMinValue) *
					(mTrack->getWidth() - mHandle->getWidth())));
			}
		}

		void _cursorMoved(const Ogre::Vector2& cursorPos)
		{
			if (mDragging)
			{
				Ogre::Vector2 co = Widget::cursorOffset(mHandle, cursorPos);
				Ogre::Real newLeft = mHandle->getLeft() + co.x - mDragOffset;
				Ogre::Real rightBoundary = mTrack->getWidth() - mHandle->getWidth();

				mHandle->setLeft(Ogre::Math::Clamp<int>((int)newLeft, 0, (int)rightBoundary));
				setValue(getSnappedValue(newLeft / rightBoundary));
			}
		}

		void _focusLost()
		{
			mDragging = false;
		}

	protected:

		/*-----------------------------------------------------------------------------
		| Internal method - given a percentage (from left to right), gets the
		| value of the nearest marker.
		-----------------------------------------------------------------------------*/
		Ogre::Real getSnappedValue(Ogre::Real percentage)
		{
			percentage = Ogre::Math::Clamp<Ogre::Real>(percentage, 0, 1);
			unsigned int whichMarker = (unsigned int) (percentage * (mMaxValue - mMinValue) / mInterval + 0.5);
			return whichMarker * mInterval + mMinValue;
		}

		Ogre::TextAreaOverlayElement* mTextArea;
		Ogre::TextAreaOverlayElement* mYesArea;
		Ogre::TextAreaOverlayElement* mNoArea;

		Ogre::BorderPanelOverlayElement* mTrack;
		Ogre::PanelOverlayElement* mHandle;
		bool mDragging;
		bool mFitToContents;
		Ogre::Real mDragOffset;
		Ogre::Real mValue;
		Ogre::Real mMinValue;
		Ogre::Real mMaxValue;
		Ogre::Real mInterval;
	};

using namespace Ogre;

const uint m_Width = 640;
const uint m_Height = 480;

// Note: wont work as expected for > 5 users in scene
static unsigned int g_UsersColors[] = {/*0x70707080*/0 ,0x80FF0000,0x80FF4500,0x80FF1493,0x8000ff00, 0x8000ced1,0x80ffd700};
#define GetColorForUser(i) g_UsersColors[(i)%(sizeof(g_UsersColors)/sizeof(unsigned int))]

#define VALIDATE_GENERATOR(type, desc, generator)				\
{																\
	rc = m_Context.EnumerateExistingNodes(nodes, type);			\
	if (nodes.IsEmpty())										\
{															\
	printf("No %s generator!\n", desc);						\
	return 1;												\
}															\
	(*(nodes.Begin())).GetInstance(generator);					\
}
#define CHECK_RC(rc, what)											\
	if (rc != XN_STATUS_OK)											\
{																\
	printf("%s failed: %s\n", what, xnGetStatusString(rc));		\
	return rc;													\
}
using namespace OgreBites;

YesNoSlider* createYesNoSlider(TrayLocation trayLoc, const Ogre::String& name, const Ogre::DisplayString& caption,
	Ogre::Real width, Ogre::Real trackWidth, Ogre::Real minValue, Ogre::Real maxValue, unsigned int snaps,SdkTrayManager* mTryMngr)
		{
			YesNoSlider* s = new YesNoSlider(name, caption, width, trackWidth,  minValue, maxValue, snaps);
			mTryMngr->moveWidgetToTray(s, trayLoc);
			//s->_assignListener(mListener);
			return s;
		}

class KinectController
{
private:

	// all the animations our character has, and a null ID
	// some of these affect separate body parts and will be blended together


public:
	xn::Context m_Context;
#if SHOW_DEPTH
	xn::DepthGenerator m_DepthGenerator;
#endif
	xn::UserGenerator m_UserGenerator;
	xn::HandsGenerator m_HandsGenerator;
	xn::GestureGenerator m_GestureGenerator;
	xn::SceneAnalyzer m_SceneAnalyzer;

	XnVSessionManager* m_pSessionManager;
	XnVFlowRouter* m_pQuitFlow;
	XnVSelectableSlider1D* m_pQuitSSlider;
	HandTracker* m_HandTracker;
	double m_SmoothingFactor;
	int m_SmoothingDelta;

	bool m_front;	
	bool suppress;

		OgreBites::ParamsPanel* m_help;
	YesNoSlider* m_quitSlider;
	OgreBites::SdkTrayManager *m_pTrayMgr;

	Vector3 m_origTorsoPos;
	XnUserID m_candidateID;

	StartPoseDetector * m_pStartPoseDetector;
	EndPoseDetector * m_pEndPoseDetector;

	XnCallbackHandle m_hPoseCallbacks;
	XnCallbackHandle m_hUserCallbacks;
	XnCallbackHandle m_hCalibrationCallbacks;
	


	static void XN_CALLBACK_TYPE NewUser(xn::UserGenerator& generator, const XnUserID nUserId, void* pCookie)
	{
		// start looking for calibration pose for new users
		generator.GetPoseDetectionCap().StartPoseDetection("Psi", nUserId);
	}

	static  void XN_CALLBACK_TYPE LostUser(xn::UserGenerator& generator, const XnUserID nUserId, void* pCookie)
	{
		KinectController* This = (KinectController*)pCookie;
		if(This->m_candidateID == nUserId )
		{
			This->m_candidateID = 0;
			This->m_pEndPoseDetector->SetUserId(0);
			This->m_pStartPoseDetector->Reset();
		}
	}

	static  void XN_CALLBACK_TYPE CalibrationStart(xn::SkeletonCapability& skeleton, const XnUserID nUserId, void* pCookie)
	{
	}

	static  void XN_CALLBACK_TYPE CalibrationEnd(xn::SkeletonCapability& skeleton, const XnUserID nUserId, XnBool bSuccess, void* pCookie)
	{
		KinectController* This = (KinectController*)pCookie;

		if (bSuccess)
		{
			// start tracking
			skeleton.StartTracking(nUserId);
			
			This->m_pStartPoseDetector->SetStartPoseState(true);
			This->m_pEndPoseDetector->SetUserId(nUserId);

			// save torso position
			XnSkeletonJointPosition torsoPos;
			skeleton.GetSkeletonJointPosition(nUserId, XN_SKEL_TORSO, torsoPos);
			This->m_origTorsoPos.x = -torsoPos.position.X;
			This->m_origTorsoPos.y = torsoPos.position.Y;
			This->m_origTorsoPos.z = -torsoPos.position.Z;

			This->m_pQuitFlow->SetActive(NULL);

			This->suppress = true;

			This->m_HandTracker->initROI(This->m_candidateID);
		}
		else
		{
			This->m_candidateID = 0;
		}
	}

	static void XN_CALLBACK_TYPE PoseDetected(xn::PoseDetectionCapability& poseDetection, const XnChar* strPose, XnUserID nId, void* pCookie)
	{
		KinectController* This = (KinectController*)pCookie;

		// If we dont have an active candidate
		if(This->m_candidateID == 0)
		{
			This->m_candidateID = nId;
			This->m_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
			This->m_pStartPoseDetector->SetStartPoseState(true);
		}
	}

	static void XN_CALLBACK_TYPE PoseLost(xn::PoseDetectionCapability& poseDetection, const XnChar* strPose, XnUserID nId, void* pCookie)
	{
		KinectController* This = (KinectController*)pCookie;
		This->m_pStartPoseDetector->Reset();
	}

	static void XN_CALLBACK_TYPE quitSSliderPPC(const XnVHandPointContext* pContext, const XnPoint3D& ptFocus, void* cxt)
	{
		KinectController* This = (KinectController*)cxt;

		if (This->suppress)
		{
			return;
		}
		This->m_pTrayMgr->moveWidgetToTray(This->m_quitSlider,OgreBites::TL_CENTER);
		This->m_quitSlider->setValue(0.5,false);
		This->m_quitSlider->show();
	}
	
	static void XN_CALLBACK_TYPE quitSSliderPPD(XnUInt32 nID, void* cxt)
	{
		KinectController* This = (KinectController*)cxt;

		This->m_pTrayMgr->moveWidgetToTray(This->m_quitSlider,OgreBites::TL_NONE);
		This->m_quitSlider->hide();
	}

	static void XN_CALLBACK_TYPE quitSSliderVC(XnFloat fValue, void* cxt)
	{
		KinectController* This = (KinectController*)cxt;

		// reverse value if we are mirrored
		if(!This->m_front)
		{
			fValue = 1-fValue;
		}

		// update quit slider visually
		This->m_quitSlider->setValue(fValue,false);

		// quit or return to demo on slider edges
		if(fValue > 0.99)
		{
			exit(0);
		} 

		else if (fValue < 0.01)
		{
			This->m_pTrayMgr->moveWidgetToTray(This->m_quitSlider,OgreBites::TL_NONE);
			This->m_quitSlider->hide();
			This->m_pSessionManager->EndSession();
		}
	}
	
	static void XN_CALLBACK_TYPE quitSSliderOAM(XnVDirection dir, void* cxt)
	{
		KinectController* This = (KinectController*)cxt;
	}

	KinectController()
	{
		XnStatus rc;
		rc = initPrimeSensor();
		m_HandTracker=new HandTracker(&m_DepthGenerator,&m_UserGenerator,m_pStartPoseDetector,m_pEndPoseDetector);
		if (XN_STATUS_OK != rc)
		{
			ErrorDialog dlg;
			dlg.display("Error initing sensor");
			exit(0);
		}
	}

	~KinectController()
	{
		m_Context.StopGeneratingAll();

		if (NULL != m_hPoseCallbacks)
		{
			m_UserGenerator.GetPoseDetectionCap().UnregisterFromPoseCallbacks(m_hPoseCallbacks);
			m_hPoseCallbacks = NULL;
		}
		if (NULL != m_hUserCallbacks)
		{
			m_UserGenerator.UnregisterUserCallbacks(m_hUserCallbacks);
			m_hUserCallbacks = NULL;
		}
		if (NULL != m_hCalibrationCallbacks)
		{
			m_UserGenerator.GetSkeletonCap().UnregisterCalibrationCallbacks(m_hCalibrationCallbacks);
			m_hCalibrationCallbacks = NULL;
		}

		m_Context.Shutdown();
	}

	XnStatus initPrimeSensor()
	{
		m_hUserCallbacks = NULL;
		m_hPoseCallbacks = NULL;
		m_hCalibrationCallbacks = NULL;

		// Init OpenNI from XML
		XnStatus rc = XN_STATUS_OK;
		rc = m_Context.InitFromXmlFile(".\\Data\\openni.xml");
		CHECK_RC(rc, "InitFromXml");

		// Make sure we have all OpenNI nodes we will be needing for this sample
		xn::NodeInfoList nodes;

#if SHOW_DEPTH
		VALIDATE_GENERATOR(XN_NODE_TYPE_DEPTH, "Depth", m_DepthGenerator);
#endif 
		VALIDATE_GENERATOR(XN_NODE_TYPE_USER, "User", m_UserGenerator);
		VALIDATE_GENERATOR(XN_NODE_TYPE_HANDS, "Gesture", m_GestureGenerator);
		VALIDATE_GENERATOR(XN_NODE_TYPE_HANDS, "Hands", m_HandsGenerator);
		
		// Init NITE Controls (UI stuff)
		m_pSessionManager = new XnVSessionManager;
		rc = m_pSessionManager->Initialize(&m_Context, "Click", "RaiseHand");
		m_pSessionManager->SetQuickRefocusTimeout(0);

		// Create quit slider & add to session manager
		m_pQuitSSlider = new XnVSelectableSlider1D(1,0,AXIS_X,1,0,500);
		m_pQuitSSlider->RegisterPrimaryPointCreate(this,quitSSliderPPC);
		m_pQuitSSlider->RegisterPrimaryPointDestroy(this,quitSSliderPPD);
		m_pQuitSSlider->RegisterOffAxisMovement(this,quitSSliderOAM);
		m_pQuitSSlider->RegisterValueChange(this,quitSSliderVC);
		
		m_pQuitFlow = new XnVFlowRouter();
		m_pQuitFlow->SetActive(m_pQuitSSlider);		
		m_pSessionManager->AddListener(m_pQuitFlow);

		suppress = false;

		// Init OpenNI nodes
		m_HandsGenerator.SetSmoothing(1);
		m_UserGenerator.RegisterUserCallbacks(KinectController::NewUser, KinectController::LostUser, this, m_hUserCallbacks);
		m_UserGenerator.GetPoseDetectionCap().RegisterToPoseCallbacks(KinectController::PoseDetected, KinectController::PoseLost, this, m_hPoseCallbacks);
#if SHOW_DEPTH
		m_DepthGenerator.GetMirrorCap().SetMirror(m_front);
#endif

		// Skeleton stuff
		m_SmoothingFactor = 0.6;
		m_SmoothingDelta = 0;
		xn::SkeletonCapability skel = m_UserGenerator.GetSkeletonCap();
		skel.SetSkeletonProfile(XN_SKEL_PROFILE_ALL);
		skel.SetSmoothing(m_SmoothingFactor);
		m_UserGenerator.GetMirrorCap().SetMirror(m_front);
		skel.RegisterCalibrationCallbacks(KinectController::CalibrationStart, KinectController::CalibrationEnd, this, m_hCalibrationCallbacks);
		
		// Make sure OpenNI nodes start generating
		rc = m_Context.StartGeneratingAll();
		CHECK_RC(rc, "StartGenerating");

		m_candidateID = 0;
		m_pStartPoseDetector = new StartPoseDetector(3.0);
		m_pEndPoseDetector = new EndPoseDetector(m_UserGenerator, 2.0);
		m_pEndPoseDetector->SetUserId(m_candidateID);

		return XN_STATUS_OK;
	}

	void addTime(Real deltaTime)
	{
		
		m_Context.WaitNoneUpdateAll();
		m_pSessionManager->Update(&m_Context);
		float r1=0,r2=0;
	}

	void injectKeyDown(const OIS::KeyEvent& evt)
	{

		

		// keep track of the player's intended direction
		//Smoothing Factor.
		if(evt.key == OIS::KC_H)
		{
			m_SmoothingDelta = 1;
		}
		else if(evt.key == OIS::KC_N)
		{
			m_SmoothingDelta = -1;
		}

	}

	void injectKeyUp(const OIS::KeyEvent& evt)
	{
		// keep track of the player's intended direction

		//Mirror.
		if(evt.key == OIS::KC_M)
		{
			m_front = !m_front;
#if SHOW_DEPTH
			m_DepthGenerator.GetMirrorCap().SetMirror(m_front);
#endif
		}

		if(evt.key == OIS::KC_H || evt.key == OIS::KC_N)
		{
			m_SmoothingDelta = 0;
		}

	}
};

#endif
