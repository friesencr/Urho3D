/**
  @class ProcSky
  @brief Procedural Sky component for Urho3D
*/

#define PROCSKY_UI
//#define PROCSKY_TEXTURE_DUMPING

#pragma once
#include <Urho3D/Urho3D.h>
#include <Urho3D/Scene/Component.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Container/Vector.h>
#include <Urho3D/Math/Vector3.h>
#include <Urho3D/Graphics/Zone.h>
using namespace Urho3D;

namespace Urho3D {
class Skybox;
class StringHash;
#if defined(PROCSKY_UI)
class UIElement;
#endif
#if defined(PROCSKY_TEXTURE_DUMPING)
class Texture2D;
class TextureCube;
#endif
}

class ProcSky: public Component {
  OBJECT(ProcSky);

public:
  ProcSky(Context* context);
  virtual ~ProcSky();
  static void RegisterObject(Context* context);
  void OnNodeSet(Node* node);

  bool GetUpdateAuto() const { return updateAuto_; }
  float GetUpdateInterval() const { return updateInterval_; }
  float GetUpdateWait() const { return updateWait_; }
  unsigned GetRenderSize() const { return renderSize_; }

  /// Automatic update renders according to update interval. If Manual, user calls Update() to render.
  void SetUpdateAuto(bool updateAuto);
  /// Set the rendering interval (default 0).
  void SetUpdateInterval(float interval) { updateInterval_ = interval; }
  /// Set size of Skybox TextureCube.
  bool SetRenderSize(unsigned size);
  /// Initialize objects and subscribe to update events.
  bool Initialize();
  /// Queue render of next frame.
  void Update();

protected:
  void HandleUpdate(StringHash eventType, VariantMap& eventData);
  /// Set rendering of next frame active/inactive.
  void SetRenderQueued(bool enable);
  void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);

#if defined(PROCSKY_UI)
  void HandleKeyDown(StringHash eventType, VariantMap& eventData);
  void ToggleUI();
  void CreateSlider(UIElement* parent, const String& label, float* target, float range);
  void HandleSliderChanged(StringHash eventType, VariantMap& eventData);
#endif

#if defined(PROCSKY_TEXTURE_DUMPING)
  void DumpTexCubeImages(TextureCube* texCube, const String& filePathPrefix);
  void DumpTexture(Texture2D* texture, const String& filePath);
#endif

protected:
  /// Camera used for face projections.
  Camera* cam_;
  /// Zone used for ibl
  Zone* zone_;
  /// Urho3D Skybox with geometry and main TextureCube.
  SharedPtr<Skybox> skybox_;
  /// Node used for light direction.
  WeakPtr<Node> lightNode_;
  SharedPtr<RenderPath> rPath_;
  /// Render size of each face.
  unsigned renderSize_;
  /// FOV used to initialize the default camera. Can adjust for Skybox seams.
  float renderFOV_;
  /// Fixed rotations for each cube face.
  Matrix3 faceRotations_[MAX_CUBEMAP_FACES];

  bool updateAuto_;
  float updateInterval_;
  float updateWait_;
  bool renderQueued_;
#if defined(PROCSKY_UI)
  Vector3 angVel_;
#endif
  /// Atmospheric parameters.
  Vector3 Kr_; // Absorption profile of air.
  float rayleighBrightness_;
  float mieBrightness_;
  float spotBrightness_;
  float scatterStrength_;
  float rayleighStrength_;
  float mieStrength_;
  float rayleighCollectionPower_;
  float mieCollectionPower_;
  float mieDistribution_;
};
