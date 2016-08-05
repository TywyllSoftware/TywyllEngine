//Copyright 2015-2016 Tomas Mikalauskas. All rights reserved.

#include <stdafx.h>

#include <TywLib\math\GLXMath.h>
#include <TywLib\geometry\JointTransform.h>
#include <TywAnimation\AnimationMacro.h>

#include "MD5Anim.h"
#include <iostream>



//=====================
MD5Anim::MD5Anim() :
	//=====================
	m_numFrames(0),
	m_frameRate(0),
	m_animLength(0),
	m_numJoints(0),
	m_numAnimatedComponents(0),
	m_fAnimTime(0.0f)
{

}

//=====================
MD5Anim::~MD5Anim()
//=====================
{

}

//===========================================
int	inline MD5Anim::GetNumFrames() const
//===========================================
{
	return m_numFrames;
}


//============================================
int	inline MD5Anim::GetNumJoints() const
//============================================
{
	return m_numJoints;
}


//==============================================
int inline MD5Anim::GetAnimLength() const
//==============================================
{
	return m_animLength;
}


//=============================================================================
bool	MD5Anim::LoadAnim(std::string fileName, std::string filePath)
//=============================================================================
{
	std::fstream file((filePath + fileName).c_str());


	if (!file.is_open())
	{
		return false;
	}

	m_name = fileName;
	std::string line;
	while (std::getline(file, line) )
	{
		char lineHeader[128];
		sscanf(line.c_str(), "%s", lineHeader);


		if(strcmp(lineHeader, "MD5Version") == 0)
		{
			
		}
		else if (strcmp(lineHeader, "numFrames") == 0)
		{
			sscanf(line.c_str(), "%s %i \n", lineHeader, &m_numFrames);
			std::cout << "numFrames " << m_numFrames << std::endl;
		}
		else if (strcmp(lineHeader, "numJoints") == 0)
		{
			sscanf(line.c_str(), "%s %i \n", lineHeader, &m_numJoints);
			std::cout << "numJoints " << m_numJoints << std::endl;
		}
		else if (strcmp(lineHeader, "frameRate") == 0)
		{
			sscanf(line.c_str(), "%s %i \n", lineHeader, &m_frameRate);
			std::cout << "frameRate " << m_frameRate << std::endl;
		}
		else if (strcmp(lineHeader, "numAnimatedComponents") == 0)
		{
			sscanf(line.c_str(), "%s %i \n", lineHeader, &m_numAnimatedComponents);
			std::cout << "numAnimatedComponents " << m_numAnimatedComponents << std::endl;
		}
		else if (strcmp(lineHeader, "hierarchy") == 0)
		{
			jointAnimInfo_t temp;
			m_jointInfo.reserve(m_numJoints);

			for (int i = 0; i < m_numJoints; i++)
			{
				std::getline(file, line);
				int rval = sscanf(line.c_str(), "%s %i %i %i \n", temp.strNameIndex, &temp.iParentNum, &temp.iAnimBits, &temp.iFirstComponent);

				if (rval == 4)
				{
					m_jointInfo.push_back(std::move(temp));
				}
			}
		}
		else if (strcmp(lineHeader, "bounds") == 0)
		{

			for (int i = 0; i < m_numFrames; i++)
			{
				std::getline(file, line);
			}
		}
		else if (strcmp(lineHeader, "baseframe") == 0)
		{
			JointQuat joint;

			m_baseFrame.reserve(m_numJoints);
			m_animatedSkeleton.joints.reserve(m_numJoints);
			for (int i = 0; i < m_numJoints; i++)
			{
				std::getline(file, line);
				int rval = sscanf(line.c_str(), " ( %f %f %f ) ( %f %f %f )", &joint.t.x, &joint.t.y, &joint.t.z, &joint.q.x, &joint.q.y, &joint.q.z);

				if (rval == 6)
				{
					joint.q.CalcW();
					m_baseFrame.push_back(joint);
					m_animatedSkeleton.joints.push_back(joint);
				}
			}
		}
		else if (strcmp(lineHeader, "frame") == 0)
		{
			for (int i = 0; i < m_numFrames; i++)
			{
				std::vector<float> frameData;
				frameData.reserve(m_numAnimatedComponents);

				std::string strTemp("");
				float fTemp;
				for (int i = 0; i < m_numAnimatedComponents; i++)
				{
					file >> fTemp;
					frameData.push_back(std::move(fTemp));
				}
				//assert(frameData.size() == m_numAnimatedComponents);

				BuildFrameSkeleton(m_skeletons, m_baseFrame, frameData, m_jointInfo);

				//shitty and ugly hack
				while (strTemp != "frame" && !file.eof())
				{
					file >> strTemp;
				}
				std::getline(file, line); //skip current line;
			}
			
		}
	}
	file.close();

//	assert(m_numJoints == m_jointInfo.size());
//	assert(m_numJoints == m_baseFrame.size());
//	assert(m_numFrames == m_skeletons.size());


	// we don't count last frame because it would cause a 1 frame pause at the end
	//m_animLength = ((m_numFrames - 1) * 1000 + m_frameRate - 1) / m_frameRate;

	m_fFrameDuration = 1.0f / (float)m_frameRate;
	m_fAnimDuration = (m_fFrameDuration * (float)m_numFrames);
	m_fAnimTime = 0.0f;

	//done
	return true;
}

//=========================
bool MD5Anim::Reload()
//=========================
{
	if (m_name.empty())
	{
		return false;
	}
	
	Clear();
	return LoadAnim(m_name, ANIMATION_LOCATION);
}


//=======================
void MD5Anim::Clear()
//=======================
{
	m_numFrames = 0;
	m_frameRate = 0;
	m_animLength = 0;
	m_numJoints = 0;
	m_numAnimatedComponents = 0;

	m_totalDelta.Clear();
	m_jointInfo.clear();
	m_baseFrame.clear();
}


//====================================================================================================================
void MD5Anim::ConvertTimeToFrame(int time, int cyclecount, frameBlend_t &frame) const
//====================================================================================================================
{
	int frameTime;
	int frameNum;

	/*
	if (m_numFrames <= 1) {
		frame.frame1 = 0;
		frame.frame2 = 0;
		frame.backlerp = 0.0f;
		frame.frontlerp = 1.0f;
		frame.cycleCount = 0;
		return;
	}


	

	if (time <= 0) {
		frame.frame1 = 0;
		frame.frame2 = 1;
		frame.backlerp = 0.0f;
		frame.frontlerp = 1.0f;
		frame.cycleCount = 0;
		return;
	}

	frameTime = time * m_frameRate;
	frameNum = frameTime / 1000;
	frame.cycleCount = frameNum / (m_numFrames - 1);

	if ((cyclecount > 0) && (frame.cycleCount >= cyclecount)) {
		frame.cycleCount = cyclecount - 1;
		frame.frame1 = m_numFrames - 1;
		frame.frame2 = frame.frame1;
		frame.backlerp = 0.0f;
		frame.frontlerp = 1.0f;
		return;
	}

	frame.frame1 = frameNum % (m_numFrames - 1);
	frame.frame2 = frame.frame1 + 1;
	if (frame.frame2 >= m_numFrames) {
		frame.frame2 = 0;
	}
	

	frame.backlerp = (frameTime % 1000) * 0.001f;
	frame.frontlerp = 1.0f - frame.backlerp;
	*/
}

void MD5Anim::ConvertDeltaTimeToFrame(float fDeltaTime, frameBlend_t &frame)
{
	m_fAnimTime += fDeltaTime;

	while (m_fAnimTime > m_animLength) m_fAnimTime -= m_fAnimDuration;
	while (m_fAnimTime < 0.0f) m_fAnimTime += m_fAnimDuration;

	// Figure out which frame we're on
	float fFramNum = m_fAnimTime * (float)m_frameRate;
	int iFrame0 = (int)floorf(fFramNum);
	int iFrame1 = (int)ceilf(fFramNum);
	iFrame0 = iFrame0 % m_numFrames;
	iFrame1 = iFrame1 % m_numFrames;

	float fInterpolate = fmodf(m_fAnimTime, m_fFrameDuration) / (float)m_fFrameDuration;

	frame.iFrame1 = iFrame0;
	frame.iFrame2 = iFrame1;
	frame.fLerp	  = fInterpolate;
}

//========================================================================================================================================
void MD5Anim::BlendJoints(FrameSkeleton& finalSkeleton, const FrameSkeleton& frame0, const FrameSkeleton& frame1, float lerp)
//========================================================================================================================================
{
	for (int i = 0; i < m_numJoints; i++)
	{
		JointQuat& finalJoint	= finalSkeleton.joints[i];
		const JointQuat& joint0 = frame0.joints[i];
		const JointQuat& joint1 = frame1.joints[i];

		finalJoint.t =  GLXVec3Lerp(joint0.t, joint1.t, lerp);
		finalJoint.q =  GLXQuatSlerp(joint0.q, joint1.q, lerp);
	}
}

//====================================================================================================================
void MD5Anim::GetInterpolatedFrame(frameBlend_t &frame)
//====================================================================================================================
{	
	if (m_numAnimatedComponents == 0) 
	{
		// just use the base frame
		return;
	}


	const FrameSkeleton& frame1 = m_skeletons[frame.iFrame1];
	const FrameSkeleton& frame2 = m_skeletons[frame.iFrame2];


	BlendJoints(m_animatedSkeleton, frame1, frame2, frame.fLerp);
}

//==============================================================================================================================
int MD5Anim::DecodeInterpolatedFrames(std::vector<JointQuat>& joints, std::vector<JointQuat>& blendJoints, std::vector<int>& lerpIndex, 
	const float * frame1, const float * frame2, const std::vector<jointAnimInfo_t> & jointInfo, const int * index, const int numIndexes)
//===============================================================================================================================
{
	int numLerpJoints = 0;
	for (int i = 0; i < numIndexes; i++)
	{
		const int j = 0;
		//const int j = index[i];
		const jointAnimInfo_t * infoPtr = &jointInfo[i];

		const int animBits = infoPtr->iAnimBits;
		if (animBits != 0)
		{
			lerpIndex[numLerpJoints++] = i;

			JointQuat * jointPtr = &joints[i];
			JointQuat * blendPtr = &blendJoints[i];

			*blendPtr = *jointPtr;

			const float * jointframe1 = frame1 + infoPtr->iFirstComponent;
			const float * jointframe2 = frame2 + infoPtr->iFirstComponent;

			if (animBits & (ANIM_TX | ANIM_TY | ANIM_TZ)) 
			{
				if (animBits & ANIM_TX) 
				{
					jointPtr->t.x = *jointframe1++;
					blendPtr->t.x = *jointframe2++;
				}
				if (animBits & ANIM_TY)
				{
					jointPtr->t.y = *jointframe1++;
					blendPtr->t.y = *jointframe2++;
				}
				if (animBits & ANIM_TZ) 
				{
					jointPtr->t.z = *jointframe1++;
					blendPtr->t.z = *jointframe2++;
				}
			}

			if (animBits & (ANIM_QX | ANIM_QY | ANIM_QZ)) 
			{
				if (animBits & ANIM_QX)
				{
					jointPtr->q.x = *jointframe1++;
					blendPtr->q.x = *jointframe2++;
				}
				if (animBits & ANIM_QY) 
				{
					jointPtr->q.y = *jointframe1++;
					blendPtr->q.y = *jointframe2++;
				}
				if (animBits & ANIM_QZ) 
				{
					jointPtr->q.z = *jointframe1++;
					blendPtr->q.z = *jointframe2++;
				}
				jointPtr->q.CalcW();
				blendPtr->q.CalcW();
			}
		}
	}
	return numLerpJoints;
}

//=========================================================================================================
void MD5Anim::GetSingleFrame(int framenum, JointQuat *joints, const int *index, int numIndexes) 
//=========================================================================================================
{
	if (framenum == 0 || numIndexes == 0)
	{
		//just use base frame
		return;
	}

	//const float* frame = &m_componentFrames[framenum * m_numAnimatedComponents];

	//DecodeSingleFrame(joints, frame, m_jointInfo, index, numIndexes);
}

//==================================================================================================
void MD5Anim::DecodeSingleFrame(JointQuat * joints, const float * frame, 
	const std::vector<jointAnimInfo_t> & jointInfo, const int * index, const int numIndexes)
//==================================================================================================
{
	for (int i = 0; i < numIndexes; i++)
	{
		const int j = 0;
		const jointAnimInfo_t * infoPtr = &jointInfo[j];

		const int animBits = infoPtr->iAnimBits;
		if (animBits != 0)
		{
			JointQuat * jointPtr = &joints[j];

			const float * jointframe = frame + infoPtr->iFirstComponent;

			if (animBits & (ANIM_TX | ANIM_TY | ANIM_TZ)) 
			{
				if (animBits & ANIM_TX) 
				{
					jointPtr->t.x = *jointframe++;
				}
				if (animBits & ANIM_TY)
				{
					jointPtr->t.y = *jointframe++;
				}
				if (animBits & ANIM_TZ) 
				{
					jointPtr->t.z = *jointframe++;
				}
			}

			if (animBits & (ANIM_QX | ANIM_QY | ANIM_QZ)) 
			{
				if (animBits & ANIM_QX) 
				{
					jointPtr->q.x = *jointframe++;
				}
				if (animBits & ANIM_QY) 
				{
					jointPtr->q.y = *jointframe++;
				}
				if (animBits & ANIM_QZ) 
				{
					jointPtr->q.z = *jointframe++;
				}
				jointPtr->q.CalcW();
			}
		}
	}
}

void MD5Anim::BuildFrameSkeleton(FrameSkeletonList& skeletons, std::vector<JointQuat>& baseFrame, std::vector<float> & frameData, std::vector<jointAnimInfo_t>&	jointInfos)
{

	FrameSkeleton skeleton;
	for (int i = 0; i < jointInfos.size(); i++)
	{
		unsigned int j = 0;

		const jointAnimInfo_t& jointInfo = jointInfos[i];
		JointQuat animatedJoint = baseFrame[i];

		const int animBits = jointInfo.iAnimBits;
		if (animBits != 0)
		{
			if (animBits & (ANIM_TX | ANIM_TY | ANIM_TZ))
			{
				if (animBits & ANIM_TX)
				{
					animatedJoint.t.x = frameData[jointInfo.iFirstComponent + j++];
				}
				if (animBits & ANIM_TY)
				{
					animatedJoint.t.y = frameData[jointInfo.iFirstComponent + j++];
				}
				if (animBits & ANIM_TZ)
				{
					animatedJoint.t.z = frameData[jointInfo.iFirstComponent + j++];
				}
			}

			if (animBits & (ANIM_QX | ANIM_QY | ANIM_QZ))
			{
				if (animBits & ANIM_QX)
				{
					animatedJoint.q.x = frameData[jointInfo.iFirstComponent + j++];
				}
				if (animBits & ANIM_QY)
				{
					animatedJoint.q.y = frameData[jointInfo.iFirstComponent + j++];
				}
				if (animBits & ANIM_QZ)
				{
					animatedJoint.q.z = frameData[jointInfo.iFirstComponent + j++];
				}
				animatedJoint.q.CalcW();
			}
		}

		if (jointInfo.iParentNum >= 0)
		{
			JointQuat& parentJoint = skeleton.joints[jointInfo.iParentNum];

			JointMat mat;

			mat.SetRotation(parentJoint.q.ToMat4());
			mat.SetTranslation(parentJoint.t);

			glx::vec3<float> rotPos = mat * animatedJoint.t;

			animatedJoint.t = parentJoint.t + rotPos;
			animatedJoint.q = parentJoint.q * animatedJoint.q;

			GLXQuatNormalize(animatedJoint.q);
		}
		skeleton.joints.push_back(std::move(animatedJoint));
	}
	skeletons.push_back(std::move(skeleton));
}