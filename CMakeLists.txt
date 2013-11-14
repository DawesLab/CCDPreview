cmake_minimum_required(VERSION 2.8)
project( CCDPreview )
find_package( OpenCV REQUIRED )
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package( Boost 1.40 COMPONENTS program_options REQUIRED)

add_executable( CCDPreview CCDPreview.cpp )

include_directories( ${Boost_INCLUDE_DIRS} )
target_link_libraries( CCDPreview ${Boost_LIBRARIES} )
target_link_libraries( CCDPreview ${OpenCV_LIBRARIES} )
target_link_libraries( CCDPreview picam )
include_directories( "/opt/PrincetonInstruments/picam/includes" )

