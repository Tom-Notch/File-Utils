# File Utils

This repository is a ROS library-only package that provides file utilities.

Pre-built docker image contains all the dependencies and can be used as dev/deploy image, it is multi-arch and can run on both x86 and ARM devices.

## Dependencies

System environment needs to have the following environment:

- ROS (Noetic preferred)
- GoogleTest
- Glog
- Gflags
- python3-opencv (for python lib)

## Usage

Put this under the `src` folder of your catkin workspace, and modify your dependent `package.xml`:

```xml
<!-- ... something like <buildtool_depend>catkin</buildtool_depend> ... -->
<depend>file_utils</depend>
<!-- ... other dependencies ... -->
```

Modify its `CMakeLists.txt` accordingly:

```cmake
# ...
find_package(
  catkin REQUIRED COMPONENTS ... file_utils ...)
# ...

# ...
# Need to add the following to solve some linking issues
# Glog
find_package(Glog REQUIRED)
list(APPEND CATKIN_SYSTEM_DEPENDS_LIST GLOG) # some common practice to simplify catkin_package(... DEPENDS ...)
list(APPEND INCLUDE_LIST ${GLOG_INCLUDE_DIRS})
list(APPEND LIB_LIST ${GLOG_LIBRARIES})
# ...

# ...
# modify the catkin_package to include file_utils under the CATKIN_DEPENDS
catkin_package(
  INCLUDE_DIRS
  ...
  include
  ...
  LIBRARIES
  ${PROJECT_NAME}
  CATKIN_DEPENDS
  ...
  file_utils
  ...
  DEPENDS
  ...)
# ...
```
