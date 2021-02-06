install(FILES
  NTSC/data_fx.properties
  NTSC/etv.properties
  NTSC/obscure.properties
  DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/feeds/NTSC
)

install(FILES
  PAL/border.properties
  PAL/data_fx.properties
  PAL/etv.properties
  PAL/example.properties
  PAL/obscure.properties
  DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/feeds/PAL
)
