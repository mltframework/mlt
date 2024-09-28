add_subdirectory(core)

if(MOD_AVFORMAT)
  add_subdirectory(avformat)
endif()

if(MOD_DECKLINK)
  add_subdirectory(decklink)
endif()

if(MOD_FREI0R)
  add_subdirectory(frei0r)
endif()

if(MOD_GDK)
  add_subdirectory(gdk)
endif()

if(MOD_GLAXNIMATE OR MOD_GLAXNIMATE_QT6)
  add_subdirectory(glaxnimate)
endif()

if(MOD_JACKRACK)
  add_subdirectory(jackrack)
endif()

if(MOD_KDENLIVE)
  add_subdirectory(kdenlive)
endif()

if(MOD_NDI)
  add_subdirectory(ndi)
endif()

if(MOD_NORMALIZE)
  add_subdirectory(normalize)
endif()

if(MOD_OLDFILM)
  add_subdirectory(oldfilm)
endif()

if(MOD_OPENCV)
  add_subdirectory(opencv)
endif()

if(MOD_MOVIT)
  add_subdirectory(movit)
endif()

if(MOD_PLUS)
  add_subdirectory(plus)
endif()

if(MOD_PLUSGPL)
  add_subdirectory(plusgpl)
endif()

if(MOD_QT OR MOD_QT6)
  add_subdirectory(qt)
endif()

if(MOD_RESAMPLE)
  add_subdirectory(resample)
endif()

if(MOD_RTAUDIO)
  add_subdirectory(rtaudio)
endif()

if(MOD_RUBBERBAND)
  add_subdirectory(rubberband)
endif()

if(MOD_SDL1)
  add_subdirectory(sdl)
endif()

if(MOD_SDL2)
  add_subdirectory(sdl2)
endif()

if(MOD_SOX)
  add_subdirectory(sox)
endif()

if(MOD_SPATIALAUDIO)
  add_subdirectory(spatialaudio)
endif()

if(MOD_VIDSTAB)
  add_subdirectory(vid.stab)
endif()

if(MOD_OPENFX)
  add_subdirectory(openfx)
endif()

if(MOD_VORBIS)
  add_subdirectory(vorbis)
endif()

if(MOD_XINE)
  add_subdirectory(xine)
endif()

if(MOD_XML)
  add_subdirectory(xml)
endif()
