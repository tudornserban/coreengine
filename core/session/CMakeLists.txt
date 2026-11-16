# core.session — lobby, matchmaking, offline resolution, online sync.
set(CORE_SESSION_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/InProcessHost.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Latency.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OfflineResolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OnlineMatch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SessionClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SessionCodec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SessionServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SessionTypes.cpp
)

add_library(coreengine_session OBJECT ${CORE_SESSION_SOURCES})
target_include_directories(coreengine_session PUBLIC ${CMAKE_SOURCE_DIR})
