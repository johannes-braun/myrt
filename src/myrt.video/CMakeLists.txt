add_executable(myrt.video
  main.cpp)

target_link_libraries(myrt.video PUBLIC
  myrt.sfml)

find_library(AVCODEC_LIBRARY avcodec)
find_library(AVDEVICE_LIBRARY avdevice)
find_library(AVFILTER_LIBRARY avfilter)
find_library(AVFORMAT_LIBRARY avformat)
find_library(AVUTIL_LIBRARY avutil)
find_library(SWRESAMPLE_LIBRARY swresample)
find_library(SWSCALE_LIBRARY swscale)
target_link_libraries(myrt.video PUBLIC 
  ${AVCODEC_LIBRARY} 
  ${AVDEVICE_LIBRARY} 
  ${AVFILTER_LIBRARY} 
  ${AVFORMAT_LIBRARY} 
  ${AVUTIL_LIBRARY} 
  ${SWRESAMPLE_LIBRARY} 
  ${SWSCALE_LIBRARY})
target_link_libraries(myrt.video PRIVATE rnu::rnu)