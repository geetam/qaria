cmake_minimum_required(VERSION 2.8.11)
project(qaria)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Find the QtWidgets library
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Sql REQUIRED)
#find_package(Qt5 REQUIRED COMPONENTS Core Widgets Sql)
set(qaria_SRC
  src/main.cpp
  src/qaria.cpp
  src/dialognewdownload.cpp
  src/progressdelegate.cpp
)

# Create code from a list of Qt designer ui files.
#set(CMAKE_AUTOUIC ON) # use this if you have CMake 3.x instead of the following
qt5_wrap_ui(qaria_SRC src/qaria.ui src/dialognewdownload.ui)

# Tell CMake to create the helloworld executable
add_executable(qaria ${qaria_SRC})

# Use the Widgets module from Qt 5.
target_link_libraries(qaria Qt5::Widgets Qt5Sql aria2)

# Install the executable
install(TARGETS qaria DESTINATION bin)
