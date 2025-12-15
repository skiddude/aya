add_library(NetworkPlayer STATIC $<TARGET_OBJECTS:RakNet>)
add_library(NetworkStudio STATIC $<TARGET_OBJECTS:RakNet>)
add_library(NetworkServer STATIC $<TARGET_OBJECTS:RakNet>)

set(SOURCES
  src/API.cpp
  src/BoostAppend.cpp
  src/BoostAppend.hpp
  src/ChatFilter.cpp
  src/ChatFilter.hpp
  src/Client.cpp
  src/Client.hpp
  src/ClientReplicator.cpp
  src/ClientReplicator.hpp
  src/ClusterUpdateBuffer.cpp
  src/ClusterUpdateBuffer.hpp
  src/Compressor.cpp
  src/Compressor.hpp
  src/CrispProxy.hpp
  src/ConcurrentRakPeer.cpp
  src/ConcurrentRakPeer.hpp
  src/DataBlockEncryptor.cpp
  src/DataBlockEncryptor.hpp
  src/Dictionary.cpp
  src/Dictionary.hpp
  src/DirectPhysicsReceiver.cpp
  src/DirectPhysicsReceiver.hpp
  src/ErrorCompPhysicsSender.cpp
  src/ErrorCompPhysicsSender.hpp
  src/ErrorCompPhysicsSender2.cpp
  src/ErrorCompPhysicsSender2.hpp
  src/GameConfigurer.cpp
  src/GameConfigurer.hpp
  src/GamePerfMonitor.cpp
  src/GamePerfMonitor.hpp
  src/GuidRegistryService.cpp
  src/GuidRegistryService.hpp
  src/InterpolatingPhysicsReceiver.cpp
  src/InterpolatingPhysicsReceiver.hpp
  src/Item.cpp
  src/Item.hpp
  src/Marker.cpp
  src/Marker.hpp
  src/MechanismItem.cpp
  src/MechanismItem.hpp
  src/MovementHistoryJob.cpp
  src/MovementHistoryJob.hpp
  src/NetworkClusterPacketCache.cpp
  src/NetworkFilter.cpp
  src/NetworkFilter.hpp
  src/NetworkOwnerJob.cpp
  src/NetworkOwnerJob.hpp
  src/NetworkPacketCache.cpp
  src/NetworkProfiler.cpp
  src/NetworkProfiler.hpp
  src/NetworkSettings.cpp
  src/NetworkSettings.hpp
  src/PacketIds.hpp
  src/Peer.cpp
  src/Peer.hpp
  src/PersistentDataStore.cpp
  src/PersistentDataStore.hpp
  src/PhysicsReceiver.cpp
  src/PhysicsReceiver.hpp
  src/PhysicsSender.cpp
  src/PhysicsSender.hpp
  src/Player.cpp
  src/Players.cpp
  src/PropertySynchronization.hpp
  src/Replicator.ChangePropertyItem.cpp
  src/Replicator.ChangePropertyItem.hpp
  src/Replicator.cpp
  src/Replicator.DeleteInstanceItem.cpp
  src/Replicator.DeleteInstanceItem.hpp
  src/Replicator.EventInvocationItem.cpp
  src/Replicator.EventInvocationItem.hpp
  src/Replicator.GCJob.cpp
  src/Replicator.GCJob.hpp
  src/Replicator.hpp
  src/Replicator.ItemSender.cpp
  src/Replicator.ItemSender.hpp
  src/Replicator.JoinDataItem.cpp
  src/Replicator.JoinDataItem.hpp
  src/Replicator.MarkerItem.cpp
  src/Replicator.MarkerItem.hpp
  src/Replicator.NewInstanceItem.cpp
  src/Replicator.NewInstanceItem.hpp
  src/Replicator.PingBackItem.cpp
  src/Replicator.PingBackItem.hpp
  src/Replicator.PingItem.cpp
  src/Replicator.PingItem.hpp
  src/Replicator.PingJob.hpp
  src/Replicator.ProcessPacketsJob.hpp
  src/Replicator.ReferencePropertyChangedItem.cpp
  src/Replicator.ReferencePropertyChangedItem.hpp
  src/Replicator.SendDataJob.hpp
  src/Replicator.StatsItem.cpp
  src/Replicator.StatsItem.hpp
  src/Replicator.StreamJob.cpp
  src/Replicator.StreamJob.hpp
  src/Replicator.TagItem.cpp
  src/Replicator.TagItem.hpp
  src/ReplicatorStats.cpp
  src/ReplicatorStats.hpp
  src/Rijndael-Boxes.hpp
  src/rijndael.cpp
  src/Rijndael.hpp
  src/RoundRobinPhysicsSender.cpp
  src/RoundRobinPhysicsSender.hpp
  src/Server.cpp
  src/Server.hpp
  src/ServerReplicator.cpp
  src/ServerReplicator.hpp
  src/Streaming.cpp
  src/Streaming.hpp
  src/StreamingUtil.hpp
  src/TopNErrorsPhysicsSender.cpp
  src/TopNErrorsPhysicsSender.hpp
  src/Util.cpp
  src/Util.hpp
  src/WebChatFilter.cpp
  src/WebChatFilter.hpp
)

set(PRECOMPILED_HEADERS
  src/API.hpp
  src/NetworkClusterPacketCache.hpp
  src/NetworkOwner.hpp
  src/NetworkPacketCache.hpp
  src/Player.hpp
  src/Players.hpp
)

if(AYA_OS_WINDOWS)
  list(APPEND SOURCES
    src/CrashReporter.cpp
    src/CrashReporter.hpp
    src/RakNetFast.hpp
  )
endif()

target_include_directories(NetworkPlayer PUBLIC src PRIVATE ${THIRD_PARTY_DIR}/BulletPhysics/src ${THIRD_PARTY_DIR}/RakNet/src ${ENGINE_DIR}/app/src ${ENGINE_DIR}/core/src ${ENGINE_DIR}/3d/src ${ENGINE_DIR}/gfx/src)
target_include_directories(NetworkStudio PUBLIC src PRIVATE ${THIRD_PARTY_DIR}/BulletPhysics/src ${THIRD_PARTY_DIR}/RakNet/src ${ENGINE_DIR}/app/src ${ENGINE_DIR}/core/src ${ENGINE_DIR}/3d/src ${ENGINE_DIR}/gfx/src)
target_include_directories(NetworkServer PUBLIC src PRIVATE ${THIRD_PARTY_DIR}/BulletPhysics/src ${THIRD_PARTY_DIR}/RakNet/src ${ENGINE_DIR}/app/src ${ENGINE_DIR}/core/src ${ENGINE_DIR}/3d/src ${ENGINE_DIR}/gfx/src)

target_precompile_headers(NetworkPlayer PUBLIC ${PRECOMPILED_HEADERS})
target_precompile_headers(NetworkStudio PUBLIC ${PRECOMPILED_HEADERS})
target_precompile_headers(NetworkServer PUBLIC ${PRECOMPILED_HEADERS})

target_sources(NetworkPlayer PRIVATE ${SOURCES})
target_sources(NetworkStudio PRIVATE ${SOURCES})
target_sources(NetworkServer PRIVATE ${SOURCES})

target_link_libraries(NetworkPlayer PRIVATE $<TARGET_OBJECTS:AppPlayer>)
target_link_libraries(NetworkStudio PRIVATE $<TARGET_OBJECTS:AppStudio>)
target_link_libraries(NetworkServer PRIVATE $<TARGET_OBJECTS:AppServer>)