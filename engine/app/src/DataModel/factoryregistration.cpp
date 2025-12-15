


#include "DataModel/factoryregistration.hpp"

#include "FastLog.hpp"

#include "DataModel/Adornment.hpp"
#include "DataModel/BillboardGui.hpp"
#include "DataModel/SurfaceGui.hpp"
#include "DataModel/Accoutrement.hpp"
#include "DataModel/Backpack.hpp"
#include "DataModel/BadgeService.hpp"
#include "DataModel/BasicPartInstance.hpp"
#include "DataModel/MeshPartInstance.hpp"
#include "DataModel/BevelMesh.hpp"
#include "DataModel/BlockMesh.hpp"
#include "DataModel/CacheableContentProvider.hpp"
#include "DataModel/Camera.hpp"
#include "DataModel/ChangeHistory.hpp"
#include "DataModel/CharacterAppearance.hpp"
#include "DataModel/CharacterMesh.hpp"
#include "DataModel/ChatService.hpp"
#include "DataModel/ClickDetector.hpp"
#include "DataModel/ContentProvider.hpp"
#include "DataModel/Configuration.hpp"
#include "DataModel/CollectionService.hpp"
#include "DataModel/CSGDictionaryService.hpp"
#include "DataModel/CustomEvent.hpp"
#include "DataModel/CustomEventReceiver.hpp"
#include "DataModel/CylinderMesh.hpp"
#include "DataModel/VirtualUser.hpp"
#include "DataModel/LogService.hpp"
#include "DataModel/DataModelMesh.hpp"
#include "DataModel/DebrisService.hpp"
#include "DataModel/Decal.hpp"
#include "DataModel/DialogRoot.hpp"
#include "DataModel/DialogChoice.hpp"
#include "DataModel/DebugSettings.hpp"
#include "DataModel/PhysicsSettings.hpp"
#include "DataModel/ExtrudedPartInstance.hpp"
#include "DataModel/FriendService.hpp"
#include "DataModel/Folder.hpp"
#include "DataModel/RenderHooksService.hpp"
#include "DataModel/Test.hpp"
#include "DataModel/TeleportService.hpp"
#include "DataModel/PersonalServerService.hpp"
#include "DataModel/UserInputService.hpp"
#include "DataModel/AssetService.hpp"
#include "DataModel/HttpService.hpp"
#include "DataModel/HttpRbxApiService.hpp"
#include "DataModel/DataStoreService.hpp"
#include "DataModel/TerrainRegion.hpp"
#include "DataModel/PathfindingService.hpp"
#include "DataModel/StarterPlayerService.hpp"
#include "DataModel/HandleAdornment.hpp"

#include "DataModel/PrismInstance.hpp"
#include "DataModel/PyramidInstance.hpp"
#include "DataModel/ParallelRampInstance.hpp"
#include "DataModel/RightAngleRampInstance.hpp"
#include "DataModel/CornerWedgeInstance.hpp"
#include "DataModel/ThumbnailGenerator.hpp"
#include "DataModel/Explosion.hpp"
#include "DataModel/FaceInstance.hpp"
#include "DataModel/Feature.hpp"
#include "DataModel/FileMesh.hpp"
#include "DataModel/Fire.hpp"
#include "DataModel/Flag.hpp"
#include "DataModel/FlagStand.hpp"
#include "DataModel/FlyweightService.hpp"
#include "DataModel/ForceField.hpp"
#include "DataModel/GameSettings.hpp"
#include "DataModel/GameBasicSettings.hpp"
#include "Script/LuaSettings.hpp"
#include "Script/DebuggerManager.hpp"
#include "Script/ModuleScript.hpp"
#include "Script/LuaSourceContainer.hpp"
#include "DataModel/GeometryService.hpp"
#include "DataModel/GlobalSettings.hpp"
#include "DataModel/Gyro.hpp"
#include "DataModel/Handles.hpp"
#include "DataModel/HandlesBase.hpp"
#include "DataModel/ArcHandles.hpp"
#include "DataModel/Hopper.hpp"
#include "DataModel/JointInstance.hpp"
#include "DataModel/JointsService.hpp"
#include "DataModel/Lighting.hpp"
#include "DataModel/MeshContentProvider.hpp"
#include "DataModel/Message.hpp"
#include "DataModel/Hint.hpp"
#include "DataModel/Mouse.hpp"
#include "DataModel/NonReplicatedCSGDictionaryService.hpp"
#include "DataModel/ParametricPartInstance.hpp"
#include "DataModel/PartInstance.hpp"
#include "DataModel/PhysicsService.hpp"
#include "DataModel/Platform.hpp"
#include "DataModel/PlayerGui.hpp"
#include "DataModel/PlayerScripts.hpp"
#include "DataModel/PlayerMouse.hpp"
#include "DataModel/PluginManager.hpp"
#include "DataModel/PluginMouse.hpp"
#include "DataModel/Seat.hpp"
#include "DataModel/SelectionBox.hpp"
#include "DataModel/SelectionSphere.hpp"
#include "DataModel/Sky.hpp"
#include "DataModel/SkateboardPlatform.hpp"
#include "DataModel/SkateboardController.hpp"
#include "DataModel/Smoke.hpp"
#include "DataModel/CustomParticleEmitter.hpp"
#include "DataModel/SolidModelContentProvider.hpp"
#include "DataModel/Sparkles.hpp"
#include "DataModel/SpawnLocation.hpp"
#include "DataModel/SpecialMesh.hpp"
#include "DataModel/Stats.hpp"
#include "DataModel/SurfaceSelection.hpp"
#include "DataModel/Teams.hpp"
#include "DataModel/TextService.hpp"
#include "DataModel/TextureContentProvider.hpp"
#include "DataModel/TimerService.hpp"
#include "DataModel/Tool.hpp"
#include "DataModel/StudioTool.hpp"
#include "DataModel/TouchTransmitter.hpp"
#include "DataModel/UserController.hpp"
#include "DataModel/Value.hpp"
#include "DataModel/VehicleSeat.hpp"
#include "DataModel/Visit.hpp"
#include "DataModel/Workspace.hpp"
#include "DataModel/LocalWorkspace.hpp"
#include "Humanoid/Humanoid.hpp"
#include "Humanoid/StatusInstance.hpp"
#include "Humanoid/HumanoidState.hpp"
#include "Script/ScriptContext.hpp"
#include "Script/script.hpp"
#include "Script/CoreScript.hpp"
#include "DataModel/MarketplaceService.hpp"
#include "DataModel/GuiService.hpp"
#include "DataModel/GuiBase.hpp"
#include "DataModel/GuiBase3d.hpp"
#include "DataModel/TweenService.hpp"
#include "DataModel/GuiObject.hpp"
#include "DataModel/ScreenGui.hpp"
#include "DataModel/Frame.hpp"
#include "DataModel/Scale9Frame.hpp"
#include "DataModel/ImageButton.hpp"
#include "DataModel/ImageLabel.hpp"
#include "DataModel/TextButton.hpp"
#include "DataModel/TextLabel.hpp"
#include "DataModel/TextBox.hpp"
#include "DataModel/ChromiumFrame.hpp"
#include "DataModel/VideoFrame.hpp"
#include "DataModel/SelectionLasso.hpp"
#include "DataModel/TextureTrail.hpp"
#include "DataModel/FloorWire.hpp"
#include "DataModel/Animation.hpp"
#include "DataModel/AnimationController.hpp"
#include "DataModel/AnimationTrack.hpp"
#include "DataModel/AnimationTrackState.hpp"
#include "DataModel/Animator.hpp"
#include "DataModel/KeyframeSequenceProvider.hpp"
#include "DataModel/KeyframeSequence.hpp"
#include "DataModel/Keyframe.hpp"
#include "DataModel/Pose.hpp"
#include "DataModel/MegaCluster.hpp"
#include "DataModel/Bindable.hpp"
#include "DataModel/Light.hpp"
#include "DataModel/Remote.hpp"
#include "DataModel/PartOperation.hpp"
#include "DataModel/PartOperationAsset.hpp"
#include "DataModel/Attachment.hpp"
#include "DataModel/TouchInputService.hpp"
#include "DataModel/HapticService.hpp"
#include "DataModel/PostEffect.hpp"
#include "DataModel/BlurEffect.hpp"
#include "DataModel/ColorCorrectionEffect.hpp"
#include "DataModel/BloomEffect.hpp"
#include "NetworkPacketCache.hpp"
#include "NetworkClusterPacketCache.hpp"
#include "ChatFilter.hpp"

#include "Utility/Sound.hpp"
#include "Utility/SoundService.hpp"
#include "Utility/UDim.hpp"
#include "Utility/Faces.hpp"
#include "Utility/Axes.hpp"
#include "Utility/ScriptInformationProvider.hpp"
#include "Utility/Action.hpp"
#include "Utility/Region3.hpp"
#include "Utility/KeywordFilter.hpp"
#include "Utility/SystemAddress.hpp"
#include "Utility/LuaWebService.hpp"
#include "Utility/rbxrandom.hpp"
#include "Utility/RunStateOwner.hpp"
#include "Utility/PhysicalProperties.hpp"
#include "DataModel/InsertService.hpp"
#include "DataModel/SocialService.hpp"
#include "DataModel/GamePassService.hpp"
#include "DataModel/ContextActionService.hpp"
#include "DataModel/LoginService.hpp"
#include "Utility/ContentFilter.hpp"
#include "Tool/LuaDragger.hpp"
#include "Tool/AdvLuaDragger.hpp"
#include "RbxTime.hpp"
#include "DataModel/InputObject.hpp"
#include "DataModel/ReplicatedStorage.hpp"
#include "DataModel/RobloxReplicatedStorage.hpp"
#include "DataModel/ReplicatedFirst.hpp"
#include "DataModel/ServerScriptService.hpp"
#include "DataModel/ServerStorage.hpp"
#include "Utility/StandardOut.hpp"
#include "Utility/KeyCode.hpp"
#include "DataModel/PointsService.hpp"
#include "DataModel/ScrollingFrame.hpp"
#include "DataModel/AdService.hpp"
#include "DataModel/NotificationService.hpp"
#include "DataModel/GroupService.hpp"
#include "DataModel/GamepadService.hpp"

#include "DataModel/NumberSequence.hpp"
#include "DataModel/NumberRange.hpp"
#include "DataModel/ColorSequence.hpp"

#include "Utility/TweenInfo.hpp"
#include "Utility/VoiceChatOutput.hpp"

#include "Utility/AyaService.hpp"

using namespace Aya;

AYA_REGISTER_TYPE(void);
AYA_REGISTER_TYPE(bool);
AYA_REGISTER_TYPE(float);
AYA_REGISTER_TYPE(int);
AYA_REGISTER_TYPE(long);
AYA_REGISTER_TYPE(double);
AYA_REGISTER_TYPE(std::string);
AYA_REGISTER_TYPE(Aya::ProtectedString);
AYA_REGISTER_TYPE(Aya::TweenInfo);
AYA_REGISTER_TYPE(const Reflection::PropertyDescriptor*);
AYA_REGISTER_TYPE(Aya::BrickColor);
AYA_REGISTER_TYPE(Aya::SystemAddress);
AYA_REGISTER_TYPE(Aya::MeshId);
AYA_REGISTER_TYPE(Aya::AnimationId);
AYA_REGISTER_TYPE(boost::shared_ptr<const Reflection::Tuple>);
AYA_REGISTER_TYPE(G3D::Vector3);
AYA_REGISTER_TYPE(G3D::Vector3int16);
AYA_REGISTER_TYPE(Aya::Region3);
AYA_REGISTER_TYPE(Aya::RbxRay);
AYA_REGISTER_TYPE(G3D::Rect2D);
AYA_REGISTER_TYPE(Aya::PhysicalProperties);
AYA_REGISTER_TYPE(Aya::Vector2);
AYA_REGISTER_TYPE(G3D::Vector2int16);
AYA_REGISTER_TYPE(G3D::Color3);
AYA_REGISTER_TYPE(G3D::CoordinateFrame);
AYA_REGISTER_TYPE(Aya::ContentId);
AYA_REGISTER_TYPE(Aya::TextureId);
AYA_REGISTER_TYPE(Aya::UDim);
AYA_REGISTER_TYPE(Aya::UDim2);
AYA_REGISTER_TYPE(Aya::Faces);
AYA_REGISTER_TYPE(Aya::Axes);
AYA_REGISTER_TYPE(boost::shared_ptr<const Instances>);
AYA_REGISTER_TYPE(boost::shared_ptr<class Reflection::DescribedBase>);
AYA_REGISTER_TYPE(boost::shared_ptr<class Aya::Instance>);
AYA_REGISTER_TYPE(Lua::WeakFunctionRef);
AYA_REGISTER_TYPE(shared_ptr<Lua::GenericFunction>);
AYA_REGISTER_TYPE(shared_ptr<Lua::GenericAsyncFunction>);
AYA_REGISTER_TYPE(boost::shared_ptr<const Reflection::ValueArray>);
AYA_REGISTER_TYPE(boost::shared_ptr<const Reflection::ValueMap>);
AYA_REGISTER_TYPE(boost::shared_ptr<const Reflection::ValueTable>);
AYA_REGISTER_TYPE(Soundscape::SoundId);
AYA_REGISTER_TYPE(Aya::NumberSequenceKeypoint);
AYA_REGISTER_TYPE(Aya::ColorSequenceKeypoint);
AYA_REGISTER_TYPE(Aya::NumberSequence);
AYA_REGISTER_TYPE(Aya::ColorSequence);
AYA_REGISTER_TYPE(Aya::NumberRange);
AYA_REGISTER_TYPE(Aya::Guid::Data);

AYA_REGISTER_CLASS(InputObject);
AYA_REGISTER_CLASS(TestService);
AYA_REGISTER_CLASS(FunctionalTest);
AYA_REGISTER_CLASS(Lighting);
AYA_REGISTER_CLASS(DebugSettings);
AYA_REGISTER_CLASS(PhysicsSettings);
AYA_REGISTER_CLASS(TaskSchedulerSettings);
AYA_REGISTER_CLASS(DataModel);
AYA_REGISTER_CLASS(PhysicsService);
AYA_REGISTER_CLASS(BadgeService);
AYA_REGISTER_CLASS(DialogRoot);
AYA_REGISTER_CLASS(DialogChoice);
AYA_REGISTER_CLASS(Tool);
AYA_REGISTER_CLASS(StudioTool);
AYA_REGISTER_CLASS(LuaDragger);
AYA_REGISTER_CLASS(AdvLuaDragger);
AYA_REGISTER_CLASS(Accoutrement);
AYA_REGISTER_CLASS(Backpack);
AYA_REGISTER_CLASS(BodyColors);
AYA_REGISTER_CLASS(ClickDetector);
AYA_REGISTER_CLASS(ControllerService);
AYA_REGISTER_CLASS(ChatService);
AYA_REGISTER_CLASS(TextService);
AYA_REGISTER_CLASS(VirtualUser);
AYA_REGISTER_CLASS(Explosion);
AYA_REGISTER_CLASS(Team);
AYA_REGISTER_CLASS(Instance);
AYA_REGISTER_CLASS(Flag);
AYA_REGISTER_CLASS(FlagStand);
AYA_REGISTER_CLASS(FlagStandService);
AYA_REGISTER_CLASS(ForceField);
AYA_REGISTER_CLASS(Fire);
AYA_REGISTER_CLASS(GameSettings);
AYA_REGISTER_CLASS(GameBasicSettings);
AYA_REGISTER_CLASS(GeometryService);
AYA_REGISTER_CLASS(Settings);
AYA_REGISTER_CLASS(GlobalAdvancedSettings);
AYA_REGISTER_CLASS(GlobalBasicSettings);
AYA_REGISTER_CLASS(Hat);
AYA_REGISTER_CLASS(Accessory);
AYA_REGISTER_CLASS(Hint);
AYA_REGISTER_CLASS(Message);
AYA_REGISTER_CLASS(Humanoid);
AYA_REGISTER_CLASS(StatusInstance);
AYA_REGISTER_CLASS(RunService);
AYA_REGISTER_CLASS(LegacyHopperService);
AYA_REGISTER_CLASS(LocalScript);
AYA_REGISTER_CLASS(LocalWorkspace);
AYA_REGISTER_CLASS(LuaSettings);
AYA_REGISTER_CLASS(CoreScript);
AYA_REGISTER_CLASS(Selection);
AYA_REGISTER_CLASS(ShirtGraphic);
AYA_REGISTER_CLASS(Shirt);
AYA_REGISTER_CLASS(Pants);
AYA_REGISTER_CLASS(Smoke);
AYA_REGISTER_CLASS(CustomParticleEmitter);
AYA_REGISTER_CLASS(Sparkles);
AYA_REGISTER_CLASS(StarterPackService);
AYA_REGISTER_CLASS(StarterPlayerService);
AYA_REGISTER_CLASS(StarterGuiService);
AYA_REGISTER_CLASS(UserInputService);
AYA_REGISTER_CLASS(CoreGuiService);
AYA_REGISTER_CLASS(StarterGear);
AYA_REGISTER_CLASS(Visit);
AYA_REGISTER_CLASS(ObjectValue);
AYA_REGISTER_CLASS(IntValue);
AYA_REGISTER_CLASS(DoubleValue);
AYA_REGISTER_CLASS(BoolValue);
AYA_REGISTER_CLASS(StringValue);
AYA_REGISTER_CLASS(BinaryStringValue);
AYA_REGISTER_CLASS(Vector3Value);
AYA_REGISTER_CLASS(RayValue);
AYA_REGISTER_CLASS(CFrameValue);
AYA_REGISTER_CLASS(Color3Value);
AYA_REGISTER_CLASS(BrickColorValue);
AYA_REGISTER_CLASS(IntConstrainedValue);
AYA_REGISTER_CLASS(DoubleConstrainedValue);
AYA_REGISTER_CLASS(Aya::Platform);
AYA_REGISTER_CLASS(SkateboardPlatform);
AYA_REGISTER_CLASS(Seat);
AYA_REGISTER_CLASS(VehicleSeat);
AYA_REGISTER_CLASS(DebrisService);
AYA_REGISTER_CLASS(TimerService);
AYA_REGISTER_CLASS(SpawnerService);
AYA_REGISTER_CLASS(ContentFilter);
AYA_REGISTER_CLASS(InsertService);
AYA_REGISTER_CLASS(LuaWebService);
AYA_REGISTER_CLASS(FriendService);
AYA_REGISTER_CLASS(RenderHooksService);
AYA_REGISTER_CLASS(SocialService);
AYA_REGISTER_CLASS(GamePassService);
AYA_REGISTER_CLASS(MarketplaceService);
AYA_REGISTER_CLASS(GroupService);
AYA_REGISTER_CLASS(ContextActionService);
AYA_REGISTER_CLASS(PersonalServerService);
AYA_REGISTER_CLASS(AssetService);
AYA_REGISTER_CLASS(ContentProvider);
AYA_REGISTER_CLASS(MeshContentProvider);
AYA_REGISTER_CLASS(TextureContentProvider);
AYA_REGISTER_CLASS(SolidModelContentProvider);
AYA_REGISTER_CLASS(CacheableContentProvider);
AYA_REGISTER_CLASS(ChangeHistoryService);
AYA_REGISTER_CLASS(HttpService);
AYA_REGISTER_CLASS(HttpRbxApiService);
AYA_REGISTER_CLASS(DataStoreService);
AYA_REGISTER_CLASS(PathfindingService);
AYA_REGISTER_CLASS(Path);
AYA_REGISTER_CLASS(Clothing);
AYA_REGISTER_CLASS(Skin);
AYA_REGISTER_CLASS(CharacterMesh);
AYA_REGISTER_CLASS(DataModelMesh);
AYA_REGISTER_CLASS(FileMesh);
AYA_REGISTER_CLASS(SpecialShape);
AYA_REGISTER_CLASS(BevelMesh);
AYA_REGISTER_CLASS(BlockMesh);
AYA_REGISTER_CLASS(CylinderMesh);
// AYA_REGISTER_CLASS(EggMesh);
AYA_REGISTER_CLASS(ServiceProvider);
AYA_REGISTER_CLASS(RootInstance);
AYA_REGISTER_CLASS(ModelInstance);
AYA_REGISTER_CLASS(BaseScript);
AYA_REGISTER_CLASS(Script);
AYA_REGISTER_CLASS(ScriptContext);
AYA_REGISTER_CLASS(RuntimeScriptService);
AYA_REGISTER_CLASS(ScriptInformationProvider);
AYA_REGISTER_CLASS(Workspace);
AYA_REGISTER_CLASS(Controller);
AYA_REGISTER_CLASS(HumanoidController);
AYA_REGISTER_CLASS(VehicleController);
AYA_REGISTER_CLASS(SkateboardController);
AYA_REGISTER_CLASS(Pose);
AYA_REGISTER_CLASS(Keyframe);
AYA_REGISTER_CLASS(KeyframeSequence);
AYA_REGISTER_CLASS(KeyframeSequenceProvider);
AYA_REGISTER_CLASS(Animation);
AYA_REGISTER_CLASS(AnimationController);
AYA_REGISTER_CLASS(AnimationTrack);
AYA_REGISTER_CLASS(AnimationTrackState);
AYA_REGISTER_CLASS(Animator);
AYA_REGISTER_CLASS(TeleportService);
AYA_REGISTER_CLASS(CharacterAppearance);
AYA_REGISTER_CLASS(LogService);
AYA_REGISTER_CLASS(ScrollingFrame);
AYA_REGISTER_CLASS(FlyweightService);
AYA_REGISTER_CLASS(CSGDictionaryService);
AYA_REGISTER_CLASS(NonReplicatedCSGDictionaryService);
AYA_REGISTER_CLASS(TouchInputService);


// network
AYA_REGISTER_CLASS(Network::PhysicsPacketCache);
AYA_REGISTER_CLASS(Network::InstancePacketCache);
AYA_REGISTER_CLASS(Network::ClusterPacketCache);
AYA_REGISTER_CLASS(Network::OneQuarterClusterPacketCache);
AYA_REGISTER_CLASS(Network::ChatFilter);

// Joints - in alpha order
AYA_REGISTER_CLASS(JointsService);
AYA_REGISTER_CLASS(Glue);
AYA_REGISTER_CLASS(Motor);
AYA_REGISTER_CLASS(Motor6D);
AYA_REGISTER_CLASS(Rotate);
AYA_REGISTER_CLASS(RotateP);
AYA_REGISTER_CLASS(RotateV);
AYA_REGISTER_CLASS(Snap);
AYA_REGISTER_CLASS(Weld);
AYA_REGISTER_CLASS(ManualSurfaceJointInstance);
AYA_REGISTER_CLASS(ManualWeld);
AYA_REGISTER_CLASS(ManualGlue);
AYA_REGISTER_CLASS(BodyMover);
AYA_REGISTER_CLASS(TouchTransmitter);
AYA_REGISTER_CLASS(FaceInstance);
AYA_REGISTER_CLASS(Sky);
AYA_REGISTER_CLASS(PVInstance);
AYA_REGISTER_CLASS(VelocityMotor);
AYA_REGISTER_CLASS(Feature);
AYA_REGISTER_CLASS(DynamicRotate);
AYA_REGISTER_CLASS(JointInstance);
AYA_REGISTER_CLASS(Attachment);
AYA_REGISTER_CLASS(SpawnLocation);
AYA_REGISTER_CLASS(Mouse);
AYA_REGISTER_CLASS(PlayerMouse);
AYA_REGISTER_CLASS(Teams);
AYA_REGISTER_CLASS(BackpackItem);
AYA_REGISTER_CLASS(HopperBin);
AYA_REGISTER_CLASS(Camera);
AYA_REGISTER_CLASS(BasePlayerGui);
AYA_REGISTER_CLASS(PlayerGui);
AYA_REGISTER_CLASS(PlayerScripts);
AYA_REGISTER_CLASS(StarterPlayerScripts);
AYA_REGISTER_CLASS(StarterCharacterScripts);
AYA_REGISTER_CLASS(PartInstance);
AYA_REGISTER_CLASS(MeshPartInstance);
AYA_REGISTER_CLASS(FormFactorPart);
AYA_REGISTER_CLASS(BasicPartInstance);
AYA_REGISTER_CLASS(ExtrudedPartInstance);
AYA_REGISTER_CLASS(PART::Wedge);
AYA_REGISTER_CLASS(Decal);
AYA_REGISTER_CLASS(DecalTexture);
AYA_REGISTER_CLASS(TweenService);
AYA_REGISTER_CLASS(TweenBase);
AYA_REGISTER_CLASS(GuiItem);
AYA_REGISTER_CLASS(GuiBase);
AYA_REGISTER_CLASS(GuiBase2d);
AYA_REGISTER_CLASS(GuiBase3d);
AYA_REGISTER_CLASS(GuiRoot);
AYA_REGISTER_CLASS(GuiObject);
AYA_REGISTER_CLASS(GuiButton);
AYA_REGISTER_CLASS(GuiLabel);
AYA_REGISTER_CLASS(GuiMain);
AYA_REGISTER_CLASS(GuiLayerCollector);
AYA_REGISTER_CLASS(BillboardGui);
AYA_REGISTER_CLASS(ScreenGui);
AYA_REGISTER_CLASS(SurfaceGui);
AYA_REGISTER_CLASS(SelectionLasso);
AYA_REGISTER_CLASS(SelectionPartLasso);
AYA_REGISTER_CLASS(SelectionPointLasso);
AYA_REGISTER_CLASS(TextureTrail);
AYA_REGISTER_CLASS(FloorWire);
AYA_REGISTER_CLASS(GuiService);
AYA_REGISTER_CLASS(Frame);
AYA_REGISTER_CLASS(Scale9Frame);
AYA_REGISTER_CLASS(GuiImageButton);
AYA_REGISTER_CLASS(ImageLabel);

AYA_REGISTER_CLASS(PostEffect);
AYA_REGISTER_CLASS(BlurEffect);
AYA_REGISTER_CLASS(ColorCorrectionEffect);
AYA_REGISTER_CLASS(BloomEffect);

AYA_REGISTER_CLASS(ChromiumFrame);
#ifdef ENABLE_VOICE_CHAT
AYA_REGISTER_CLASS(VoiceChatOutput);
#endif
AYA_REGISTER_CLASS(AyaService);
AYA_REGISTER_CLASS(VideoFrame);

AYA_REGISTER_CLASS(GuiTextButton);
AYA_REGISTER_CLASS(TextBox);
AYA_REGISTER_CLASS(TextLabel);
AYA_REGISTER_CLASS(PartAdornment);
AYA_REGISTER_CLASS(PVAdornment);
AYA_REGISTER_CLASS(Handles);
AYA_REGISTER_CLASS(HandlesBase);
AYA_REGISTER_CLASS(ArcHandles);
AYA_REGISTER_CLASS(SelectionBox);
AYA_REGISTER_CLASS(SelectionSphere);
AYA_REGISTER_CLASS(HandleAdornment);
AYA_REGISTER_CLASS(BoxHandleAdornment);
AYA_REGISTER_CLASS(ConeHandleAdornment);
AYA_REGISTER_CLASS(CylinderHandleAdornment);
AYA_REGISTER_CLASS(SphereHandleAdornment);
AYA_REGISTER_CLASS(LineHandleAdornment);
AYA_REGISTER_CLASS(ImageHandleAdornment);
AYA_REGISTER_CLASS(SurfaceSelection);
AYA_REGISTER_CLASS(CollectionService);
AYA_REGISTER_CLASS(Configuration);
AYA_REGISTER_CLASS(Folder);
AYA_REGISTER_CLASS(MotorFeature);
AYA_REGISTER_CLASS(Hole);
AYA_REGISTER_CLASS(MegaClusterInstance);
AYA_REGISTER_CLASS(PluginMouse);
AYA_REGISTER_CLASS(PluginManager);
AYA_REGISTER_CLASS(Plugin);
AYA_REGISTER_CLASS(Toolbar);
AYA_REGISTER_CLASS(Aya::Button);
AYA_REGISTER_CLASS(PrismInstance);
AYA_REGISTER_CLASS(PyramidInstance);
AYA_REGISTER_CLASS(ParallelRampInstance);
AYA_REGISTER_CLASS(RightAngleRampInstance);
AYA_REGISTER_CLASS(CornerWedgeInstance);
AYA_REGISTER_CLASS(CustomEvent);
AYA_REGISTER_CLASS(CustomEventReceiver);
// AYA_REGISTER_CLASS(PropertyInstance);
AYA_REGISTER_CLASS(BindableFunction);
AYA_REGISTER_CLASS(BindableEvent);
AYA_REGISTER_CLASS(Aya::Scripting::DebuggerManager);
AYA_REGISTER_CLASS(Aya::Scripting::ScriptDebugger);
AYA_REGISTER_CLASS(Aya::Scripting::DebuggerBreakpoint);
AYA_REGISTER_CLASS(Aya::Scripting::DebuggerWatch);
AYA_REGISTER_CLASS(Light);
AYA_REGISTER_CLASS(PointLight);
AYA_REGISTER_CLASS(SpotLight);
AYA_REGISTER_CLASS(SurfaceLight);
AYA_REGISTER_CLASS(LoginService);
AYA_REGISTER_CLASS(ReplicatedStorage);
AYA_REGISTER_CLASS(RobloxReplicatedStorage);
AYA_REGISTER_CLASS(ServerScriptService);
AYA_REGISTER_CLASS(ServerStorage);
AYA_REGISTER_CLASS(RemoteFunction);
AYA_REGISTER_CLASS(RemoteEvent);
AYA_REGISTER_CLASS(TerrainRegion);
AYA_REGISTER_CLASS(ModuleScript);
AYA_REGISTER_CLASS(PointsService);
AYA_REGISTER_CLASS(AdService);
AYA_REGISTER_CLASS(NotificationService);
AYA_REGISTER_CLASS(ReplicatedFirst);
AYA_REGISTER_CLASS(PartOperation);
AYA_REGISTER_CLASS(PartOperationAsset);
AYA_REGISTER_CLASS(UnionOperation);
AYA_REGISTER_CLASS(NegateOperation);
AYA_REGISTER_CLASS(Soundscape::SoundService);
AYA_REGISTER_CLASS(Soundscape::SoundChannel);
AYA_REGISTER_CLASS(GamepadService);
AYA_REGISTER_CLASS(LuaSourceContainer);
AYA_REGISTER_CLASS(HapticService);
AYA_REGISTER_CLASS(ThumbnailGenerator);

// Xbox
#if defined(AYA_PLATFORM_DURANGO)
#include "DataModel/PlatformService.hpp"
AYA_REGISTER_CLASS(PlatformService);
#endif

static void onSlotException(std::exception& ex)
{
    FASTLOG(FLog::Error, "Slot Exception");
    Aya::StandardOut::singleton()->printf(MESSAGE_ERROR, "exception while signalling: %s", ex.what());
}

FactoryRegistrator::FactoryRegistrator()
{
    G3D::System::time(); // Initialize the Program Start Time.
    registerSound();
    Aya::registerScriptDescriptors();
    registerBodyMovers();

    registerValueClasses();
    Aya::registerStatsClasses();
    Aya::Surface::registerSurfaceDescriptors();

    Aya::signals::slot_exception_handler = onSlotException;

    srand(Aya::randomSeed());

    ModelInstance::hackPhysicalCharacter();
}

// Enum types
AYA_REGISTER_ENUM(ChangeHistoryService::RuntimeUndoBehavior);
AYA_REGISTER_ENUM(FunctionalTest::Result);
AYA_REGISTER_ENUM(TaskScheduler::PriorityMethod);
AYA_REGISTER_ENUM(TaskScheduler::Job::SleepAdjustMethod);
AYA_REGISTER_ENUM(TaskScheduler::ThreadPoolConfig);
AYA_REGISTER_ENUM(Action::ActionType);
AYA_REGISTER_ENUM(Controller::Button);
AYA_REGISTER_ENUM(HopperBin::BinType);
AYA_REGISTER_ENUM(GuiObject::SizeConstraint);
AYA_REGISTER_ENUM(PlayBackState);
AYA_REGISTER_ENUM(TweenInfo::TweenEasingStyle);
AYA_REGISTER_ENUM(TweenInfo::TweenStatus);
AYA_REGISTER_ENUM(TweenInfo::TweenEasingDirection);
AYA_REGISTER_ENUM(TextService::XAlignment);
AYA_REGISTER_ENUM(TextService::YAlignment);
AYA_REGISTER_ENUM(TextService::FontSize);
AYA_REGISTER_ENUM(TextService::Font);
AYA_REGISTER_ENUM(Camera::CameraType);
AYA_REGISTER_ENUM(Camera::CameraMode);
AYA_REGISTER_ENUM(Camera::CameraPanMode);
AYA_REGISTER_ENUM(LegacyController::InputType);
AYA_REGISTER_ENUM(DataModelArbiter::ConcurrencyModel);
AYA_REGISTER_ENUM(DataModelMesh::LODType);
AYA_REGISTER_ENUM(DebugSettings::ErrorReporting);
AYA_REGISTER_ENUM(EThrottle::EThrottleType);
AYA_REGISTER_ENUM(Feature::InOut);
AYA_REGISTER_ENUM(Feature::LeftRight);
AYA_REGISTER_ENUM(Feature::TopBottom);
AYA_REGISTER_ENUM(Joint::JointType);
AYA_REGISTER_ENUM(KeywordFilterType);
AYA_REGISTER_ENUM(Legacy::SurfaceConstraint);
AYA_REGISTER_ENUM(NormalId);
AYA_REGISTER_ENUM(Vector3::Axis);
AYA_REGISTER_ENUM(Humanoid::Status);
AYA_REGISTER_ENUM(Humanoid::HumanoidRigType);
AYA_REGISTER_ENUM(Humanoid::NameOcclusion);
AYA_REGISTER_ENUM(Humanoid::HumanoidDisplayDistanceType);
AYA_REGISTER_ENUM(Aya::HUMAN::StateType);
AYA_REGISTER_ENUM(DataModel::CreatorType);
AYA_REGISTER_ENUM(DataModel::Genre);
AYA_REGISTER_ENUM(DataModel::GearGenreSetting);
AYA_REGISTER_ENUM(DataModel::GearType);
AYA_REGISTER_ENUM(Instance::SaveFilter);
AYA_REGISTER_ENUM(BasicPartInstance::LegacyPartType);
AYA_REGISTER_ENUM(KeyframeSequence::Priority);
AYA_REGISTER_ENUM(SocialService::StuffType);
AYA_REGISTER_ENUM(PersonalServerService::PrivilegeType);
AYA_REGISTER_ENUM(ExtrudedPartInstance::VisualTrussStyle);
AYA_REGISTER_ENUM(PrismInstance::NumSidesEnum);
AYA_REGISTER_ENUM(PyramidInstance::NumSidesEnum);
AYA_REGISTER_ENUM(FriendService::FriendStatus);
AYA_REGISTER_ENUM(FriendService::FriendEventType);
AYA_REGISTER_ENUM(Handles::VisualStyle);
AYA_REGISTER_ENUM(SkateboardPlatform::MoveState);
AYA_REGISTER_ENUM(SoundType);
AYA_REGISTER_ENUM(SpecialShape::MeshType);
AYA_REGISTER_ENUM(SurfaceType);
AYA_REGISTER_ENUM(PartInstance::FormFactor);
AYA_REGISTER_ENUM(CollisionFidelity);
AYA_REGISTER_ENUM(UserInputService::SwipeDirection);
AYA_REGISTER_ENUM(UserInputService::Platform);
AYA_REGISTER_ENUM(UserInputService::MouseType);
AYA_REGISTER_ENUM(PartMaterial);
AYA_REGISTER_ENUM(PhysicalPropertiesMode);
AYA_REGISTER_ENUM(NetworkOwnership);
AYA_REGISTER_ENUM(Time::SampleMethod);
AYA_REGISTER_ENUM(GuiService::SpecialKey);
AYA_REGISTER_ENUM(GuiService::CenterDialogType);
AYA_REGISTER_ENUM(GuiService::UiMessageType);
AYA_REGISTER_ENUM(ChatService::ChatColor);
AYA_REGISTER_ENUM(MarketplaceService::CurrencyType);
AYA_REGISTER_ENUM(MarketplaceService::InfoType);
AYA_REGISTER_ENUM(CharacterMesh::BodyPart);
AYA_REGISTER_ENUM(GameBasicSettings::ControlMode);
AYA_REGISTER_ENUM(GameBasicSettings::MaxFramerate);
AYA_REGISTER_ENUM(GameBasicSettings::RenderQualitySetting);
AYA_REGISTER_ENUM(GameBasicSettings::CameraMode);
AYA_REGISTER_ENUM(GameBasicSettings::TouchCameraMovementMode);
AYA_REGISTER_ENUM(GameBasicSettings::ComputerCameraMovementMode);
AYA_REGISTER_ENUM(GameBasicSettings::TouchMovementMode);
AYA_REGISTER_ENUM(GameBasicSettings::ComputerMovementMode);
AYA_REGISTER_ENUM(GameBasicSettings::RotationType);
AYA_REGISTER_ENUM(GameBasicSettings::VirtualVersion);
AYA_REGISTER_ENUM(Frame::Style);
AYA_REGISTER_ENUM(GuiButton::Style);
AYA_REGISTER_ENUM(DialogRoot::DialogPurpose);
AYA_REGISTER_ENUM(DialogRoot::DialogTone);
AYA_REGISTER_ENUM(Voxel::CellMaterial);
AYA_REGISTER_ENUM(Voxel::CellBlock);
AYA_REGISTER_ENUM(Voxel::CellOrientation);
AYA_REGISTER_ENUM(Voxel::WaterCellForce);
AYA_REGISTER_ENUM(Voxel::WaterCellDirection);
AYA_REGISTER_ENUM(Explosion::ExplosionType);
AYA_REGISTER_ENUM(InputObject::UserInputType);
AYA_REGISTER_ENUM(InputObject::UserInputState);
AYA_REGISTER_ENUM(AssetService::AccessType);
AYA_REGISTER_ENUM(HttpService::HttpContentType);
AYA_REGISTER_ENUM(StarterGuiService::CoreGuiType);
AYA_REGISTER_ENUM(StarterPlayerService::DeveloperTouchCameraMovementMode);
AYA_REGISTER_ENUM(StarterPlayerService::DeveloperComputerCameraMovementMode);
AYA_REGISTER_ENUM(StarterPlayerService::DeveloperCameraOcclusionMode);
AYA_REGISTER_ENUM(StarterPlayerService::DeveloperTouchMovementMode);
AYA_REGISTER_ENUM(StarterPlayerService::DeveloperComputerMovementMode);
AYA_REGISTER_ENUM(TeleportService::TeleportState);
AYA_REGISTER_ENUM(TeleportService::TeleportType);
AYA_REGISTER_ENUM(KeyCode);
AYA_REGISTER_ENUM(MessageType);
AYA_REGISTER_ENUM(MarketplaceService::ProductPurchaseDecision);
AYA_REGISTER_ENUM(ThrottlingPriority);
AYA_REGISTER_ENUM(Soundscape::ReverbType);
AYA_REGISTER_ENUM(Soundscape::ListenerType);
AYA_REGISTER_ENUM(Soundscape::RollOffMode);
AYA_REGISTER_ENUM(PlayerActionType);
AYA_REGISTER_ENUM(RunService::RenderPriority);
AYA_REGISTER_ENUM(AdvArrowToolBase::JointCreationMode);
AYA_REGISTER_ENUM(GuiObject::ImageScale);
AYA_REGISTER_ENUM(UserInputService::OverrideMouseIconBehavior);
AYA_REGISTER_ENUM(Pose::PoseEasingStyle);
AYA_REGISTER_ENUM(Pose::PoseEasingDirection);
AYA_REGISTER_ENUM(HapticService::VibrationMotor);
AYA_REGISTER_ENUM(UserInputService::UserCFrame);

#if defined(AYA_PLATFORM_DURANGO)
AYA_REGISTER_ENUM(XboxKeyBoardType)
AYA_REGISTER_ENUM(VoiceChatState)
#endif
