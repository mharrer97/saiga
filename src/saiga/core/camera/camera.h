/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include "saiga/core/math/math.h"

#include <saiga/core/geometry/object3d.h>
#include <saiga/core/geometry/plane.h>
#include <saiga/core/geometry/sphere.h>

namespace Saiga
{
struct ViewPort
{
    ivec2 position;
    ivec2 size;
    ViewPort() = default;
    ViewPort(ivec2 position, ivec2 size) : position(position), size(size) {}

    vec4 getVec4() const { return vec4(position(0), position(1), size(0), size(1)); }
};



class SAIGA_CORE_API Camera : public Object3D
{
   public:
    std::string name;

    //    mat4 model;
    mat4 view     = mat4::Identity();
    mat4 proj     = mat4::Identity();
    mat4 viewProj = mat4::Identity();


    float zNear, zFar;
    //    float nw,nh,fw,fh; //dimensions of near and far plane

    vec3 vertices[8];       // corners of the truncated pyramid
    Plane planes[6];        // for exact frustum culling
    Sphere boundingSphere;  // for fast frustum culling

    bool vulkanTransform = false;

    Camera();
    virtual ~Camera() {}



    void setView(const mat4& v);
    void setView(const vec3& eye, const vec3& center, const vec3& up);


    void setProj(const mat4& p);
    //    virtual void setProj( double fovy, double aspect, double zNear, double zFar){}
    //    virtual void setProj( float left, float right,float bottom,float top,float near,  float far){}

    void updateFromModel();
    void recalculateMatrices();
    virtual void recalculatePlanes();

    // linearize the depth (for rendering)
    float linearDepth(float d);
    float nonlinearDepth(float l);

    float toViewDepth(float d);
    float toNormalizedDepth(float d);


    enum IntersectionResult
    {
        OUTSIDE = 0,
        INSIDE,
        INTERSECT
    };

    // culling stuff
    IntersectionResult pointInFrustum(const vec3& p);
    IntersectionResult sphereInFrustum(const Sphere& s);

    IntersectionResult pointInSphereFrustum(const vec3& p);
    IntersectionResult sphereInSphereFrustum(const Sphere& s);

    /**
     * Return the intervall (min,max) when all vertices of the frustum are
     * projected to the axis 'd'. To dedect an overlap in intervalls the axis
     * does not have to be normalized.
     *
     * @brief projectedIntervall
     * @param d
     * @return
     */
    vec2 projectedIntervall(const vec3& d);

    /**
     * Returns the side of the plane on which the frustum is.
     * +1 on the positive side
     * -1 on the negative side
     * 0 the plane is intersecting the frustum
     *
     * @brief sideOfPlane
     * @param plane
     * @return
     */
    int sideOfPlane(const Plane& plane);

    /**
     * Exact frustum-frustum intersection with the Separating Axes Theorem (SAT).
     * This test is expensive, so it should be only used when important.
     *
     * Number of Operations:
     * 6+6=12  sideOfPlane(const Plane &plane), for testing the faces of the frustum.
     * 6*6*2=72  projectedIntervall(const vec3 &d), for testing all cross product of pairs of non parallel edges
     *
     * http://www.geometrictools.com/Documentation/MethodOfSeparatingAxes.pdf
     * @brief intersectSAT
     * @param other
     * @return
     */

    bool intersectSAT(Camera* other);


    /**
     * Calculates the frustum planes by backprojecting the unit cube to world space.
     */

    void recalculatePlanesFromMatrices();

    /**
     * Returns unique edges of the frustum.
     * A frustum has 6 unique edges ( non parallel edges).
     * @brief getEdge
     * @param i has to be in range (0 ... 5)
     * @return
     */

    std::pair<vec3, vec3> getEdge(int i);

    vec3 projectToViewSpace(vec3 worldPosition);

    vec3 projectToNDC(vec3 worldPosition);

    vec2 projectToScreenSpace(vec3 worldPosition, int w, int h);

    vec3 inverseprojectToWorldSpace(vec2 ip, float depth, int w, int h);


    virtual void recomputeProj(){};

    void imgui();

   private:
    friend std::ostream& operator<<(std::ostream& os, const Camera& ca);
};

//========================= PerspectiveCamera =========================

class SAIGA_CORE_API PerspectiveCamera : public Camera
{
   public:
    float fovy, aspect;
    float tang;
    PerspectiveCamera() {}
    void setProj(float fovy, float aspect, float zNear, float zFar, bool vulkanTransform = false);
    SAIGA_CORE_API friend std::ostream& operator<<(std::ostream& os, const PerspectiveCamera& ca);

    void imgui();
    virtual void recomputeProj() override;
    virtual void recalculatePlanes() override;
};

//========================= OrthographicCamera =========================

class SAIGA_CORE_API OrthographicCamera : public Camera
{
   public:
    float left, right, bottom, top;
    OrthographicCamera() {}
    void setProj(float left, float right, float bottom, float top, float near, float far, bool vulkanTransform = false);
    void setProj(AABB bb, bool vulkanTransform = false);

    SAIGA_CORE_API friend std::ostream& operator<<(std::ostream& os, const OrthographicCamera& ca);

    void imgui();
    virtual void recomputeProj() override;
    virtual void recalculatePlanes() override;
};


/**
 * Equivalent to the uniform block defined in camera.glsl
 * This makes uploading the camera parameters more efficient, because they can be shared in multiple shaders and
 * be upload at once.
 */
struct CameraDataGLSL
{
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec4 camera_position;

    CameraDataGLSL() {}
    CameraDataGLSL(Camera* cam)
    {
        view            = cam->view;
        proj            = cam->proj;
        viewProj        = proj * view;
        camera_position = cam->position;
    }
};

}  // namespace Saiga
