#include "StdAfx.h"
#include "HandTracker.h"


#ifndef SHOW_DEPTH
#define SHOW_DEPTH 1
#endif
#ifndef SHOW_BAR
#define SHOW_BAR 0
#endif

float lastRatio=-1;
static unsigned int g_UsersColors[] = {/*0x70707080*/0 ,0x80FF0000,0x80FF4500,0x80FF1493,0x8000ff00, 0x8000ced1,0x80ffd700};
#define GetColorForUser(i) g_UsersColors[(i)%(sizeof(g_UsersColors)/sizeof(unsigned int))]
#define ZInterval 100

HandTracker::HandTracker(xn::DepthGenerator* depth,xn::UserGenerator* user,StartPoseDetector * startPoseDetector,EndPoseDetector* endPoseDetector)
{
	m_DepthGenerator=depth;
	m_UserGenerator=user;
	m_pStartPoseDetector=startPoseDetector;
	m_pEndPoseDetector=endPoseDetector;
	m_Width=640;
	m_Height=480;
	setUp=false;
}

bool HandTracker::isSetUp()
{
	return setUp;
}

void HandTracker::setUserID(XnUserID user)
{
	userID=user;

}

HandTracker::~HandTracker(void)
{
}

void HandTracker::initROI(XnUserID user)
{
	
	if (m_UserGenerator->GetSkeletonCap().IsTracking(userID))
	{
		userID=user;
		XnSkeletonJointPosition headPosition,headProjective,neckPosition,neckProjective;
		m_UserGenerator->GetSkeletonCap().GetSkeletonJointPosition(user,XN_SKEL_HEAD,headPosition);
		m_DepthGenerator->ConvertRealWorldToProjective(1,&headPosition.position,&headProjective.position);
		m_UserGenerator->GetSkeletonCap().GetSkeletonJointPosition(user,XN_SKEL_NECK,neckPosition);
		m_DepthGenerator->ConvertRealWorldToProjective(1,&neckPosition.position,&neckProjective.position);

		xn::SceneMetaData smd;
		m_UserGenerator->GetUserPixels(0, smd);
		const XnLabel* pUsersLBLs = smd.Data();
		int leftPixel=0,
			rightPixel=0,
			topPixel=0,
			downPixel=neckProjective.position.Y-headProjective.position.Y;

		int headYPos=headProjective.position.Y;
		int headXPos=headProjective.position.X;
	//	cvNamedWindow("Image:", CV_WINDOW_AUTOSIZE);

		while(user == pUsersLBLs[headYPos*m_Width + headXPos + (rightPixel++)])
		{
			if (headXPos + rightPixel>=m_Width)
				return;
		}
		while(user == pUsersLBLs[headYPos*m_Width + headXPos - (leftPixel++)])
		{
			if (headXPos - leftPixel<0)
				return;
		}
		while(user == pUsersLBLs[(headYPos-(topPixel++))*m_Width + headXPos])
		{
			if (headYPos-topPixel<0)
				return;
		}
		
		ROIWidth=rightPixel+leftPixel;
		ROIHeight=topPixel+downPixel;

		startDistance=headPosition.position.Z;
		setUp=true;
	}
}

float processHandImage(IplImage* img)
{
	CvMemStorage* m_Storage=cvCreateMemStorage(0);
	CvMemStorage* m_DefectStorage=cvCreateMemStorage(0);
	CvSeq* Contours=0;
	CvSeq* ConvexHull=0;
	CvConvexityDefect* defectArray; 
	CvSeq* defects;  

	cvFindContours(img,m_Storage,&Contours);
	cvZero(img);

	if(Contours!=0)
	{
		ConvexHull=cvConvexHull2(Contours,0, CV_COUNTER_CLOCKWISE,     0); 
		defects = cvConvexityDefects( Contours,  
								ConvexHull,  
								m_DefectStorage);  

		int nomdef = defects->total; // defect amount                
		if (nomdef>1)
		{
	           
					// Alloc memory for defect set.     
			 defectArray = (CvConvexityDefect*)malloc(sizeof(CvConvexityDefect)*nomdef);  
              
			// Get defect set.      
			cvCvtSeqToArray(defects,defectArray, CV_WHOLE_SEQ);  
			float totalDepth=0;
			for (int i=0;i<nomdef;i++)
			{
				totalDepth+=defectArray[i].depth;
			}
			lastRatio=totalDepth/nomdef;
			free(defectArray);

		}
		if(ConvexHull!=0)
			cvClearSeq(ConvexHull);
		if(defects!=0)
			cvClearSeq(defects);
		cvClearSeq(Contours);
		
	}
	//cvClearMemStorage(m_Storage);
	//cvClearMemStorage(m_DefectStorage);
	cvReleaseMemStorage(&m_Storage);
	cvReleaseMemStorage(&m_DefectStorage);

	return lastRatio;
}

float HandTracker::UpdateDepthTexture(bool m_front)
	{
		TexturePtr texture = TextureManager::getSingleton().getByName("MyDepthTexture");
		// Get the pixel buffer
		HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();

		// Lock the pixel buffer and get a pixel box
		pixelBuffer->lock(HardwareBuffer::HBL_DISCARD); 
		const PixelBox& pixelBox = pixelBuffer->getCurrentLock();

		unsigned char* pDest = static_cast<unsigned char*>(pixelBox.data);

	
		XnSkeletonJointPosition headPosition,headProjective;
		int centerXPos,centerYPos,ROIFixedWidth,ROIFixedHeight,ROIYNegative,ROIYPositive,
			pix_size,row_w,empty_byte;
		float handZPosition;
		IplImage* handImage=0;
		unsigned char* imgPtr=0;
		XnBool isTracking;

		if (isTracking=m_UserGenerator->GetSkeletonCap().IsTracking(userID) && setUp)
		{
			m_UserGenerator->GetSkeletonCap().GetSkeletonJointPosition(userID,XN_SKEL_RIGHT_HAND,headPosition);
			m_DepthGenerator->ConvertRealWorldToProjective(1,&headPosition.position,&headProjective.position);

			centerXPos=headProjective.position.X;
			centerYPos=headProjective.position.Y;
			handZPosition=headPosition.position.Z;

			ROIFixedWidth=ROIWidth*(startDistance/handZPosition)/2;
			ROIFixedHeight=ROIHeight*(startDistance/handZPosition)/2;
			ROIYNegative=ROIFixedHeight*3/5;
			ROIYPositive=2*ROIFixedHeight-ROIYNegative;
			if (ROIYNegative>0 && ROIYPositive>0 && ROIFixedWidth>0)
			{
				handImage=cvCreateImage(cvSize(ROIFixedWidth*2-1,ROIFixedHeight*2-1),IPL_DEPTH_8U,1);
				imgPtr=(unsigned char*)handImage->imageData;
				pix_size = 1;
				row_w=handImage->width*pix_size;
				empty_byte=handImage->widthStep-row_w;
			}
			else
				setUp=false;
		}

		// Get label map 
		xn::SceneMetaData smd;
		xn::DepthMetaData dmd;

		m_UserGenerator->GetUserPixels(0, smd);
		m_DepthGenerator->GetMetaData(dmd);

		const XnLabel* pUsersLBLs = smd.Data();
		const XnDepthPixel* pDepth = dmd.Data();

		//Drawing DepthMap
		for (size_t j = 0; j < m_Height; j++)
		{
			pDest = static_cast<unsigned char*>(pixelBox.data) + j*pixelBox.rowPitch*4;
			for(size_t i = 0; i < m_Width; i++)
			{
				// fix i if we are mirrored
				uint fixed_i = i;
				if(!m_front)
				{
					fixed_i = m_Width - i;
				}

				// determine color
				unsigned int color = GetColorForUser(pUsersLBLs[j*m_Width + fixed_i]);

				// if we have a candidate, filter out the rest
				if (userID != 0)
				{
					if  (userID == pUsersLBLs[j*m_Width + fixed_i])
					{
						color = GetColorForUser(1);
						if( j > m_Height*(1 - m_pStartPoseDetector->GetDetectionPercent()))
						{
							//highlight user
							color |= 0xFF070707;
						}
						if( j < m_Height*(m_pEndPoseDetector->GetDetectionPercent()))
						{	
							//hide user
							color &= 0x20F0F0F0;
						}


					}
					else
					{
						color = 0;
					}

				}
			
				// write to output buffer
				*((unsigned int*)pDest) = color;
				pDest+=4;
			}
		}
		//Hand Tracking
		float ratio=-1;
		float avgDepth=-1;
		if (isTracking && setUp)
		{
			unsigned int lowestX=740;
			unsigned int highestX=0;
			const XnLabel* userPtr = smd.Data();
			const XnDepthPixel* depthPtr = dmd.Data();
			for (size_t j = centerYPos-ROIYNegative+1; j < centerYPos+ROIYPositive; j++)
			{
				if (j<0 || j>=m_Height)
					continue;
				int yLevel=j*m_Width;
				for(size_t i = centerXPos-ROIFixedWidth+1; i < centerXPos+ROIFixedWidth; i++)
				{
					if (i<0 || i>=m_Width)
						continue;
					uint fixed_i = i;
					if(!m_front)
					{
						fixed_i = m_Width - i;
					}
					if  (userID == userPtr[yLevel + fixed_i] && (depthPtr[yLevel + fixed_i]>(handZPosition-ZInterval) && depthPtr[yLevel + fixed_i]<(handZPosition+ZInterval)))
					{
						(*imgPtr++)=0xF0;

						if(lowestX>fixed_i)
							lowestX=fixed_i;
						if(highestX<fixed_i)
							highestX=fixed_i;
					}
						else
						(*imgPtr++)=0x00;			
				}
				imgPtr+=empty_byte;
			}
			ratio=(float)(highestX-lowestX)/(ROIFixedWidth*2);
			
			avgDepth=processHandImage(handImage);
		//	cvShowImage("Image:", handImage);
		//	cvResizeWindow("Image:",ROIFixedWidth,ROIFixedHeight*2);
			//	  cvWaitKey(0);
			if (handImage)
				cvReleaseImage(&handImage);
		}

		// Unlock the pixel buffer
		pixelBuffer->unlock();
		//return avgDepth;
		float sum=ratio*avgDepth*avgDepth;
		return sum;
		//if (sum<0 )
		//{
		//	return HAND_UNKNOWN;
		//}
		//else if(sum==0)
		//{
		//	initROI(userID);
		//	return HAND_UNKNOWN;
		//}
		//else if (sum>1.5)
		//{
		//	return HAND_OPEN;
		//}
		//else
		//{
		//	return HAND_CLOSED;
		//}
	}