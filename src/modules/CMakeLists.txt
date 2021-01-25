add_subdirectory(core)

if(MOD_AVFORMAT AND TARGET PkgConfig::libavformat AND TARGET PkgConfig::libswscale AND TARGET PkgConfig::libavutil)
  add_subdirectory(avformat)
endif()

if(MOD_DECKLINK)
  add_subdirectory(decklink)
endif()

if(MOD_FEEDS)
  add_subdirectory(feeds)
endif()

if(MOD_FREI0R AND FREI0R_FOUND)
  add_subdirectory(frei0r)
endif()

if(MOD_GDK AND TARGET PkgConfig::GdkPixbuf)
  add_subdirectory(gdk)
endif()

if(MOD_JACKRACK AND TARGET PkgConfig::jack)
  add_subdirectory(jackrack)
endif()

if(MOD_KDENLIVE)
  add_subdirectory(kdenlive)
endif()

if(MOD_LUMAS)
  add_subdirectory(lumas)
endif()

if(MOD_MOTION_EST AND GPL)
  add_subdirectory(motion_est)
endif()

if(MOD_NORMALIZE AND GPL)
  add_subdirectory(normalize)
endif()

if(MOD_OLDFILM)
  add_subdirectory(oldfilm)
endif()

if(MOD_OPENCV AND OpenCV_tracking_FOUND)
  add_subdirectory(opencv)
endif()

if(MOD_OPENGL AND TARGET PkgConfig::movit)
  add_subdirectory(opengl)
endif()

if(MOD_PLUS)
  add_subdirectory(plus)
endif()

if(MOD_PLUSGPL AND GPL)
  add_subdirectory(plusgpl)
endif()

if(MOD_QT AND GPL AND Qt5_FOUND)
  add_subdirectory(qt)
endif()

if(MOD_RESAMPLE AND TARGET PkgConfig::samplerate)
  add_subdirectory(resample)
endif()

if(MOD_RTAUDIO)
  add_subdirectory(rtaudio)
endif()

if(MOD_RUBBERBAND AND GPL AND TARGET PkgConfig::rubberband)
  add_subdirectory(rubberband)
endif()

if(MOD_SDL1 AND TARGET PkgConfig::sdl)
  add_subdirectory(sdl)
endif()

if(MOD_SDL2 AND TARGET sdl2)
  add_subdirectory(sdl2)
endif()

if(MOD_SOX AND TARGET PkgConfig::sox)
  add_subdirectory(sox)
endif()

if(MOD_VIDSTAB AND GPL AND TARGET PkgConfig::vidstab)
  add_subdirectory(vid.stab)
endif()

if(MOD_VMFX)
  add_subdirectory(vmfx)
endif()

if(MOD_VORBIS AND TARGET PkgConfig::vorbis AND TARGET PkgConfig::vorbisfile)
  add_subdirectory(vorbis)
endif()

if(MOD_XINE AND GPL)
  add_subdirectory(xine)
endif()

if(MOD_XML AND TARGET PkgConfig::xml)
  add_subdirectory(xml)
endif()
