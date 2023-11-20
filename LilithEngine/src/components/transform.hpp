#ifndef __TRANSFORM_HPP__
#define __TRANSFORM_HPP__


#include <limits>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/ext.hpp>

class Transform {
    static constexpr glm::vec3 EULER_MAX_ANGLES = glm::vec3(6.2831853072f);
public:

    explicit Transform(const glm::vec3& translation = glm::vec3(0.f), const glm::vec3& rotation = glm::vec3(), const glm::vec3& scale = glm::vec3(1.f)) :
        _translation(translation), _eul(rotation), _scale(scale), status(EulerCorrect) {};

    explicit Transform(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale) :
        _translation(translation), _qua(rotation), _scale(scale), status(QuatCorrect) {};
    
    //Translation section
    const glm::vec3& getTranslation() const { return _translation; }
    const void setTranslation(const glm::vec3 translation) { _translation = translation; }
    const void setTranslationX(const float translation) { _translation.x = translation; }
    const void setTranslationY(const float translation) { _translation.y = translation; }
    const void setTranslationZ(const float translation) { _translation.z = translation; }
    const void addTranslation(const glm::vec3 translation) { _translation += translation; }
    const void addTranslationX(const float translation) { _translation.x += translation; }
    const void addTranslationY(const float translation) { _translation.y += translation; }
    const void addTranslationZ(const float translation) { _translation.z += translation; }

    //Rotation section
    // Rotation is expressed with two different expressions : euler angles, and quaternions.
    // In order to ease transformations and minimize computations, the class switch from one expression to the other when necessary.
    //The following methods takes care of this for us.
    const glm::vec3& getRotationEuler() {
        if (status == QuatCorrect) { _eul = glm::eulerAngles(_qua); status = BothCorrect; }
        return _eul;
    }

    const glm::quat& getRotationQuat() {
        if (status == EulerCorrect) { _qua = glm::tquat(_eul); status = BothCorrect; }
        return _qua;
    }

    const glm::mat4 getRotationMatrix() {
        if (status != EulerCorrect) {
            return glm::mat4_cast(_qua);
        }
        return glm::eulerAngleYXZ(_eul.x, _eul.y, _eul.z);
    }

    void setRotationEuler(const glm::vec3 eul) {
        _eul = glm::mod(eul, EULER_MAX_ANGLES);
        status = EulerCorrect;
    }
    void setRotationEulerX(const float x) {
        if (status == QuatCorrect) { _eul = glm::eulerAngles(_qua); }
        _eul.x = glm::mod(x, EULER_MAX_ANGLES.x);
        status = EulerCorrect;
    }
    void setRotationEulerY(const float y) {
        if (status == QuatCorrect) { _eul = glm::eulerAngles(_qua); }
        _eul.y = glm::mod(y, EULER_MAX_ANGLES.y);
        status = EulerCorrect;
    }
    void setRotationEulerZ(const float z) {
        if (status == QuatCorrect) { _eul = glm::eulerAngles(_qua); }
        _eul.z = glm::mod(z, EULER_MAX_ANGLES.z);
        status = EulerCorrect;
    }
    void addRotationEuler(const glm::vec3 eul) {
        if (status == QuatCorrect) { _eul = glm::eulerAngles(_qua); }
        _eul = glm::mod(_eul + eul, EULER_MAX_ANGLES);
        status = EulerCorrect;
    }
    void addRotationEulerX(const float x) {
        if (status == QuatCorrect) { _eul = glm::eulerAngles(_qua); }
        _eul.x = glm::mod(_eul.x + x, EULER_MAX_ANGLES.x);
        status = EulerCorrect;
    }
    void addRotationEulerY(const float y) {
        if (status == QuatCorrect) { _eul = glm::eulerAngles(_qua); }
        _eul.y = glm::mod(_eul.y + y, EULER_MAX_ANGLES.y);
        status = EulerCorrect;
    }
    void addRotationEulerZ(const float z) {
        if (status == QuatCorrect) { _eul = glm::eulerAngles(_qua); }
        _eul.z = glm::mod(_eul.z + z, EULER_MAX_ANGLES.z);
        status = EulerCorrect;
    }

    void setRotationQuat(const glm::quat qua) {
        _qua = (glm::dot(qua, qua) < std::numeric_limits<float>::epsilon()) ?
            glm::quat(1.f, 0.f, 0.f, 0.f) :
            glm::normalize(qua);
        status = QuatCorrect;
    }
    void addRotationQuatLocal(const glm::quat& qua) {
        if (status == EulerCorrect) { _qua = glm::tquat(_eul); }
        _qua *= qua;
        normalizeQuat();
        status = QuatCorrect;
    }
    void addRotationQuatGlobal(const glm::quat& qua) {
        if (status == EulerCorrect) { _qua = glm::tquat(_eul); }
        _qua = qua * _qua;
        normalizeQuat();
        status = QuatCorrect;
    }

    //Specific for camera controller.
    const void addRotationLocXZGlobY(const glm::vec3 rotationEuler) {
        const glm::vec3 localRot{ rotationEuler.x, 0.f, rotationEuler.z };
        const glm::vec3 globalRot{ 0.f, rotationEuler.y, 0.f};
        //_rotationQuat = glm::quat(globalRot) * _rotationQuat * glm::quat(localRot);
    }

    //Scale section
    const glm::vec3& getScale() const { return _scale; }
    const void setScale(const glm::vec3 scale) { _scale = scale; }
    const void setScaleX(const float scale) { _scale.x = scale; }
    const void setScaleY(const float scale) { _scale.y = scale; }
    const void setScaleZ(const float scale) { _scale.z = scale; }
    const void addScale(const glm::vec3 scale) { _scale += scale; }
    const void addScaleX(const float scale) { _scale.x += scale; }
    const void addScaleY(const float scale) { _scale.y += scale; }
    const void addScaleZ(const float scale) { _scale.z += scale; }
    const void mulScale(const glm::vec3 scale) { _scale *= scale; }
    const void mulScaleX(const float scale) { _scale.x *= scale; }
    const void mulScaleY(const float scale) { _scale.y *= scale; }
    const void mulScaleZ(const float scale) { _scale.z *= scale; }

    const glm::vec3 getForward() { //Need to be obtimized
        glm::mat3 rolMat = getRotationMatrix();
        return { rolMat[0][2], rolMat[1][2], rolMat[2][2] };
    }
    //const glm::vec3 getForward() const { return glm::transpose(glm::mat3_cast(_rotationQuat)) * glm::vec3{ 0.f, 0.f, 1.f }; }

    glm::mat4 modelMatrix() {
        glm::mat4 transMat = glm::translate(glm::mat4(1.f), _translation);
        glm::mat4 rotMat = getRotationMatrix();
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.f), _scale);
        return transMat * rotMat * scaleMat;
    }

    glm::mat3 normalMatrix() {
        glm::mat3 rotMat = getRotationMatrix();
        glm::mat3 invScaleMat = glm::scale(glm::mat4(1.f), 1.f / _scale);
        return rotMat * invScaleMat;
    }

private:
    enum {
        EulerCorrect,
        QuatCorrect,
        BothCorrect,
    } status = BothCorrect;

    glm::vec3 _scale{}, _translation{};

    void normalizeQuat() {
        _qua = (glm::dot(_qua, _qua) < std::numeric_limits<float>::epsilon()) ?
            glm::quat(1.f, 0.f, 0.f, 0.f) :
            glm::normalize(_qua);
    }
    glm::vec3 _eul{};
    glm::quat _qua{ 1.f, 0.f, 0.f, 0.f };
};

#endif
