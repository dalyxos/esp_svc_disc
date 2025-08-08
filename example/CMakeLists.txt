cmake_minimum_required(VERSION 3.16)

# Add the parent directory's components folder to the component search path
set(EXTRA_COMPONENT_DIRS "../components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp_svc_disc_example)
