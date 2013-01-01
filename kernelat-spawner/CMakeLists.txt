add_executable(kernelat-spawner kernelat-spawner.c mm.c tools.c)
target_link_libraries(kernelat-spawner pww zmq)
