find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(tst-menuregistry
    tst_menuregistry.cpp
)
target_link_libraries(tst-menuregistry PRIVATE dgm-core Qt6::Test)
add_test(NAME menuregistry COMMAND tst-menuregistry)

add_executable(tst-dbusmenutypes
    tst_dbusmenutypes.cpp
)
target_link_libraries(tst-dbusmenutypes PRIVATE dgm-core Qt6::Test)
add_test(NAME dbusmenutypes COMMAND tst-dbusmenutypes)

add_executable(tst-dbusmenuimporter
    tst_dbusmenuimporter.cpp
)
target_link_libraries(tst-dbusmenuimporter PRIVATE dgm-core Qt6::Test)
add_test(NAME dbusmenuimporter COMMAND tst-dbusmenuimporter)
