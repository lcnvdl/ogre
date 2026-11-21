/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd
Copyright (c) 2006 Matthias Fink, netAllied GmbH <matthias.fink@web.de>                             

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include "OgreStableHeaders.h"
#include "OgreShadowCameraSetupPSSM.h"

namespace Ogre
{
    //---------------------------------------------------------------------
    PSSMShadowCameraSetup::PSSMShadowCameraSetup()
        : mSplitPadding(1.0f), mCurrentIteration(0)
    {
        calculateSplitPoints(3, 100, 100000);
        setOptimalAdjustFactor(0, 5);
        setOptimalAdjustFactor(1, 1);
        setOptimalAdjustFactor(2, 0);


    }
    //---------------------------------------------------------------------
    PSSMShadowCameraSetup::~PSSMShadowCameraSetup()
    {
    }
    //---------------------------------------------------------------------
    void PSSMShadowCameraSetup::calculateSplitPoints(uint splitCount, Real nearDist, Real farDist, Real lambda)
    {
        if (splitCount < 2)
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot specify less than 2 splits", 
            "PSSMShadowCameraSetup::calculateSplitPoints");

        mSplitPoints.resize(splitCount + 1);
        mOptimalAdjustFactors.resize(splitCount);
        mSplitCount = splitCount;

        mSplitPoints[0] = nearDist;
        for (size_t i = 1; i < mSplitCount; i++)
        {
            Real fraction = (Real)i / (Real)mSplitCount;
            Real splitPoint = lambda * nearDist * Math::Pow(farDist / nearDist, fraction) +
                (1.0f - lambda) * (nearDist + fraction * (farDist - nearDist));

            mSplitPoints[i] = splitPoint;
        }
        mSplitPoints[splitCount] = farDist;

    }
    //---------------------------------------------------------------------
    void PSSMShadowCameraSetup::setSplitPoints(const SplitPointList& newSplitPoints)
    {
        if (newSplitPoints.size() < 3) // 3, not 2 since splits + 1 points
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot specify less than 2 splits", 
            "PSSMShadowCameraSetup::setSplitPoints");
        mSplitCount = static_cast<uint>(newSplitPoints.size() - 1);
        mSplitPoints = newSplitPoints;
        mOptimalAdjustFactors.resize(mSplitCount);
    }
    //---------------------------------------------------------------------
    void PSSMShadowCameraSetup::setOptimalAdjustFactor(size_t splitIndex, Real factor)
    {
        if (splitIndex >= mOptimalAdjustFactors.size())
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Split index out of range", 
            "PSSMShadowCameraSetup::setOptimalAdjustFactor");
        mOptimalAdjustFactors[splitIndex] = factor;
        
    }
    //---------------------------------------------------------------------
    Real PSSMShadowCameraSetup::getOptimalAdjustFactor() const
    {
        // simplifies the overriding of the LiSPSM opt adjust factor use
        return mOptimalAdjustFactors[mCurrentIteration];
    }
    //---------------------------------------------------------------------
    void PSSMShadowCameraSetup::getShadowCamera(const Ogre::SceneManager *sm, const Ogre::Camera *cam,
        const Ogre::Viewport *vp, const Ogre::Light *light, Ogre::Camera *texCam, size_t iteration) const
    {
        // apply the right clip distance.
        Real nearDist = mSplitPoints[iteration];
        Real farDist = mSplitPoints[iteration + 1];

        // Add a padding factor to internal distances so that the connecting split point will not have bad artifacts.
        if (iteration > 0)
        {
            nearDist -= mSplitPadding;
        }
        if (iteration < mSplitCount - 1)
        {
            farDist += mSplitPadding;
        }

        mCurrentIteration = iteration;

        // Ouch, I know this is hacky, but it's the easiest way to re-use LiSPSM / Focused
        // functionality right now without major changes
        Camera* _cam = const_cast<Camera*>(cam);
        Real oldNear = _cam->getNearClipDistance();
        Real oldFar = _cam->getFarClipDistance();
        Frustum* oldCull = _cam->getCullingFrustum();
        _cam->setNearClipDistance(nearDist);
        _cam->setFarClipDistance(farDist);
        _cam->setCullingFrustum(NULL);

        LiSPSMShadowCameraSetup::getShadowCamera(sm, cam, vp, light, texCam, iteration);

        // restore near/far
        _cam->setNearClipDistance(oldNear);
        _cam->setFarClipDistance(oldFar);
        _cam->setCullingFrustum(oldCull);

    }
    //---------------------------------------------------------------------
    void PSSMShadowCameraSetup::getCullingCamera(const SceneManager* sm, const Camera* cam, const Viewport* vp,
                                                 const Light* light, Camera* cullCam, size_t iteration) const
    {
        if (light->getType() != Light::LT_DIRECTIONAL)
        {
            DefaultShadowCameraSetup::getShadowCamera(sm, cam, vp, light, cullCam, iteration);
            return;
        }

        Real nearDist = mSplitPoints[iteration];
        Real farDist = mSplitPoints[iteration + 1];
        if (iteration > 0)
        {
            nearDist -= mSplitPadding;
        }
        if (iteration < mSplitCount - 1)
        {
            farDist += mSplitPadding;
        }

        // Same trick as getShadowCamera: temporarily narrow the camera to the split to get its corners
        Camera* splitCam = const_cast<Camera*>(cam);
        Real oldNear = splitCam->getNearClipDistance();
        Real oldFar = splitCam->getFarClipDistance();
        Frustum* oldCull = splitCam->getCullingFrustum();
        splitCam->setNearClipDistance(nearDist);
        splitCam->setFarClipDistance(farDist);
        splitCam->setCullingFrustum(NULL);

        Vector3 corners[8];
        const Vector3* worldCorners = splitCam->getWorldSpaceCorners();
        std::copy(worldCorners, worldCorners + 8, corners);

        splitCam->setNearClipDistance(oldNear);
        splitCam->setFarClipDistance(oldFar);
        splitCam->setCullingFrustum(oldCull);

        Vector3 towardsLight = -light->getDerivedDirection();
        towardsLight.normalise();
        Vector3 up = Vector3::UNIT_Y;
        if (Math::Abs(up.dotProduct(towardsLight)) >= 1.0f)
        {
            up = Vector3::UNIT_Z;
        }
        Matrix3 rot = Math::lookRotation(towardsLight, up);
        Matrix3 toLightSpace = rot.transpose();

        Vector3 minimum(Math::POS_INFINITY);
        Vector3 maximum(Math::NEG_INFINITY);
        for (const Vector3& corner : corners)
        {
            Vector3 lightSpaceCorner = toLightSpace * corner;
            minimum.makeFloor(lightSpaceCorner);
            maximum.makeCeil(lightSpaceCorner);
        }

        // Small margin so the PCF kernel at the split border still finds its casters
        const Real borderMargin = 1.02f;
        Real width = (maximum.x - minimum.x) * borderMargin;
        Real height = (maximum.y - minimum.y) * borderMargin;
        Real depth = maximum.z - minimum.z;
        Real extrusion = sm->getShadowDirectionalLightExtrusionDistance();

        Vector3 lightSpaceEye((minimum.x + maximum.x) * 0.5f, (minimum.y + maximum.y) * 0.5f, maximum.z + extrusion);
        Vector3 eye = rot * lightSpaceEye;

        cullCam->setCustomProjectionMatrix(false);
        cullCam->setProjectionType(PT_ORTHOGRAPHIC);
        cullCam->setOrthoWindow(width, height);
        cullCam->setNearClipDistance(0.1f);
        cullCam->setFarClipDistance(extrusion + depth * borderMargin + 1.0f);
        cullCam->setCustomViewMatrix(true, Math::makeViewMatrix(eye, Quaternion(rot)));
    }
}
