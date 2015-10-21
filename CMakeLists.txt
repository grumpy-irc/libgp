PROJECT (gp)
SET(CMAKE_AUTOMOC ON)

if(QT5_BUILD)
  find_package(Qt5Core REQUIRED)
  find_package(Qt5Network REQUIRED)
  set(QT_INCLUDES ${Qt5Network_INCLUDE_DIRS})
  include_directories(${QT_INCLUDES})
else()
  find_package(Qt4 REQUIRED)
endif()

file (GLOB src "*.cpp")
file (GLOB headers "*.h")
file (GLOB resources "*.qrc")

if(QT5_BUILD)
    QT5_WRAP_CPP(gp_HEADERS_MOC ${gp_HEADERS})
    QT5_ADD_RESOURCES(gp_RESOURCES_RCC ${resources})
else()
    QT4_WRAP_CPP(gp_HEADERS_MOC ${gp_HEADERS})
    QT4_ADD_RESOURCES(gp_RESOURCES_RCC ${resources})
endif()

if (NOT QT5_BUILD)
    INCLUDE(${QT_USE_FILE})
endif()


ADD_DEFINITIONS(${QT_DEFINITIONS})
ADD_DEFINITIONS( -DGP_LIBRARY -DQT_USE_QSTRINGBUILDER )

ADD_LIBRARY(gp SHARED ${src} ${headers})

if (QT5_BUILD)
    qt5_use_modules(gp Core Network)
endif()

TARGET_LINK_LIBRARIES(gp ${QT_LIBRARIES})
