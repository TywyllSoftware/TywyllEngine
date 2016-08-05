//Copyright 2015-2016 Tomas Mikalauskas. All rights reserved.
#pragma once


//forward declaration
class JointQuat;

typedef TYWANIMATION_API struct
{
	int		iFrame1;
	int		iFrame2;
	float	fLerp;
} frameBlend_t;

typedef TYWANIMATION_API struct
{
	char					strNameIndex[128];
	int						iParentNum;
	int						iAnimBits;
	int						iFirstComponent;
} jointAnimInfo_t;




class TYWANIMATION_API MD5Anim
{
public:
	float						m_fAnimTime;
public:
	// A frame skeleton stores the joints of the skeleton for a single frame.
	struct FrameSkeleton
	{
		std::vector<JointQuat>  joints;
	};
	typedef std::vector<FrameSkeleton> FrameSkeletonList;

public:
	MD5Anim();
	~MD5Anim();

	bool					LoadAnim(std::string fileName, std::string filePath);
	bool					Reload();
	void					Clear();

	int						GetNumFrames() const;
	int						GetNumJoints() const;
	int						GetAnimLength() const;

	void					GetInterpolatedFrame(frameBlend_t &frame);
	void					GetSingleFrame(int framenum, JointQuat *joints, const int *index, int numIndexes);
	void					ConvertTimeToFrame(int time, int cyclecount, frameBlend_t &frame) const;
	void					ConvertDeltaTimeToFrame(float fDeltaTime, frameBlend_t &frame);

	const FrameSkeleton&	GetSkeleton() const { return m_animatedSkeleton; }

private:
	void					BuildFrameSkeleton(FrameSkeletonList& skeletons, std::vector<JointQuat>& baseFrame, std::vector<float> & frameData, std::vector<jointAnimInfo_t>& jointInfo);
	void					DecodeSingleFrame(JointQuat * joints, const float * frame, const std::vector<jointAnimInfo_t> & jointInfo, const int * index, const int numIndexes);
	int						DecodeInterpolatedFrames(std::vector<JointQuat>& joints, std::vector<JointQuat>& blendJoints, std::vector<int>& lerpIndex, const float * frame1, const float * frame2, const std::vector<jointAnimInfo_t> & jointInfo, const int * index, const int numIndexes);
	void					BlendJoints(FrameSkeleton& finalSkeleton, const FrameSkeleton& frame0, const FrameSkeleton& frame1, float lerp);


private:
	float							m_fFrameDuration;
	float							m_fAnimDuration;
	int								m_numFrames;
	int								m_frameRate;
	int								m_animLength;
	int								m_numJoints;
	int								m_numAnimatedComponents;

	FrameSkeletonList				m_skeletons;
	FrameSkeleton					m_animatedSkeleton;

	std::vector<jointAnimInfo_t>	m_jointInfo;
	std::vector<JointQuat>			m_baseFrame;
	glx::vec3<float>				m_totalDelta;
	std::string						m_name;

};