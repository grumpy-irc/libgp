PROJECT (gp)
SET(QT_USE_QTNETWORK TRUE)
SET(CMAKE_AUTOMOC ON)

option(QT6_BUILD "Build with Qt6" false)

if(QT6_BUILD)
  find_package(Qt6Core REQUIRED)
  find_package(Qt6Network REQUIRED)
  set(QT_INCLUDES ${Qt6Network_INCLUDE_DIRS})
  include_directories(${QT_INCLUDES})
elseif(QT5_BUILD)
  find_package(Qt5Core REQUIRED)
  find_package(Qt5Network REQUIRED)
  set(QT_INCLUDES ${Qt5Network_INCLUDE_DIRS})
  include_directories(${QT_INCLUDES})
else()
  message(FATAL_ERROR "Enable QT5_BUILD or QT6_BUILD")
endif()

file (GLOB src "*.cpp")
file (GLOB headers "*.h")
file (GLOB resources "*.qrc")

if(QT6_BUILD)
    QT6_WRAP_CPP(gp_HEADERS_MOC ${gp_HEADERS})
    QT6_ADD_RESOURCES(gp_RESOURCES_RCC ${resources})
elseif(QT5_BUILD)
    QT5_WRAP_CPP(gp_HEADERS_MOC ${gp_HEADERS})
    QT5_ADD_RESOURCES(gp_RESOURCES_RCC ${resources})
else()
    message(FATAL_ERROR "Enable QT5_BUILD or QT6_BUILD")
endif()


ADD_DEFINITIONS(${QT_DEFINITIONS})
ADD_DEFINITIONS( -DGP_LIBRARY -DQT_USE_QSTRINGBUILDER )

ADD_LIBRARY(gp SHARED ${src} ${headers})
set_target_properties(gp PROPERTIES VERSION 1.0.0 SOVERSION 1)

if (QT6_BUILD)
    TARGET_LINK_LIBRARIES(gp Qt6::Core Qt6::Network)
elseif (QT5_BUILD)
    TARGET_LINK_LIBRARIES(gp Qt5::Core Qt5::Network)
endif()

TARGET_LINK_LIBRARIES(gp ${QT_LIBRARIES})

if (NOT WIN32)
  INSTALL(TARGETS gp LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
