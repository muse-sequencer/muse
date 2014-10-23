## install precompiled lv2 gtk2 helpers in module dir

install(FILES lv2Gtk2Helper32.so lv2Gtk2Helper64.so
        DESTINATION ${MusE_MODULES_DIR}
        PERMISSIONS WORLD_READ WORLD_EXECUTE OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ GROUP_EXECUTE)
