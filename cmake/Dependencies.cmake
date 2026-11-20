include(FetchContent)
set(HANDLECORE_WITH_PCAP ON)
if(NOT TARGET Lidar::HandleCore)
    FetchContent_Declare(lidar_handle_core
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/HandleCore.git
        GIT_TAG 7ad9b380fe8d58cdb00a97107e06a96f6f4ba9b0)
    FetchContent_MakeAvailable(lidar_handle_core)
endif()
if(NOT TARGET Lidar::Pcap)
    message(FATAL_ERROR "Enable HANDLECORE_WITH_PCAP before the first HandleCore configuration")
endif()
set(FAMILYCORE_WITH_QML ON)
set(FAMILYCORE_WITH_PCAP_FORMS ON)
if(NOT TARGET Lidar::TaskUI)
    FetchContent_Declare(lidar_familycore
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/TaskCore.git
        GIT_TAG abe45d68ef57f000e96fc8c5503959e0b707b0b8)
    FetchContent_MakeAvailable(lidar_familycore)
endif()
if(NOT TARGET Lidar::PcapForms)
    message(FATAL_ERROR "Enable FAMILYCORE_WITH_PCAP_FORMS before the first FamilyCore configuration")
endif()
